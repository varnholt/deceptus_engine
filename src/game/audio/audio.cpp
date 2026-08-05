#include "audio.h"

#include "framework/tools/log.h"
#include "game/config/gameconfiguration.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <ranges>
#include <string>

namespace
{
const std::string music_path = "data/music";

// resolution of the loudness envelope; 50ms is short enough to follow a wind gust swelling and
// fading but long enough that the rms of a single bucket is not dominated by the waveform itself
constexpr auto loudness_bucket_duration_s = 0.05f;

std::vector<float> computeLoudnessEnvelope(const AudioBackend::SampleData& sample_data)
{
   const auto frame_count = sample_data._sample_count / sample_data._channel_count;
   const auto frames_per_bucket = static_cast<uint64_t>(static_cast<float>(sample_data._sample_rate) * loudness_bucket_duration_s);
   const auto bucket_count = std::max<uint64_t>(1, (frame_count + frames_per_bucket - 1) / frames_per_bucket);

   std::vector<float> envelope;
   envelope.reserve(static_cast<size_t>(bucket_count));

   auto loudest = 0.0f;

   for (uint64_t bucket = 0; bucket < bucket_count; bucket++)
   {
      const auto first_frame = bucket * frames_per_bucket;
      const auto last_frame = std::min(first_frame + frames_per_bucket, frame_count);

      auto sum_of_squares = 0.0;
      for (auto frame = first_frame; frame < last_frame; frame++)
      {
         // the mean across the channels is enough here, the envelope does not need to be per-channel
         auto channel_sum = 0.0f;
         for (uint32_t channel = 0; channel < sample_data._channel_count; channel++)
         {
            channel_sum += static_cast<float>(sample_data._samples[frame * sample_data._channel_count + channel]);
         }

         const auto mean = channel_sum / static_cast<float>(sample_data._channel_count);
         sum_of_squares += static_cast<double>(mean) * static_cast<double>(mean);
      }

      const auto frames_in_bucket = last_frame - first_frame;
      const auto rms = frames_in_bucket > 0 ? std::sqrt(sum_of_squares / static_cast<double>(frames_in_bucket)) : 0.0;

      envelope.push_back(static_cast<float>(rms));
      loudest = std::max(loudest, envelope.back());
   }

   // normalize against the sample's own peak so the caller gets a 0..1 value regardless of how hot
   // the sample was mastered
   if (loudest > 0.0f)
   {
      for (auto& bucket_loudness : envelope)
      {
         bucket_loudness /= loudest;
      }
   }

   return envelope;
}
}  // namespace

/*!
 * \brief Audio::Audio
 * \param parent
 */
Audio::Audio()
{
   _backend = AudioBackend::create();
   initializeSamples();
}

/*!
 * \brief Audio::getInstance
 * \return
 */
Audio& Audio::getInstance()
{
   static Audio __instance;
   return __instance;
}

Audio::~Audio()
{
   std::lock_guard<std::mutex> guard(_mutex);
   _stopped = true;
   for (auto& thread : _sound_threads)
   {
      if (thread._sound)
      {
         thread._sound->stop();
      }
   }
}

void Audio::addSample(const std::string& sample)
{
   std::lock_guard<std::mutex> lock(_mutex);

   if (_backend->hasSample(sample))
   {
      return;
   }

   _backend->loadSample(sample);
}

std::optional<sf::Time> Audio::getSampleDuration(const std::string& sample_name)
{
   std::lock_guard<std::mutex> lock(_mutex);

   const auto sample_data = _backend->getSampleData(sample_name);
   if (!sample_data.has_value() || sample_data->_sample_rate == 0 || sample_data->_channel_count == 0)
   {
      return std::nullopt;
   }

   const auto frame_count = sample_data->_sample_count / sample_data->_channel_count;
   return sf::seconds(static_cast<float>(frame_count) / static_cast<float>(sample_data->_sample_rate));
}

const std::vector<float>* Audio::getLoudnessEnvelope(const std::string& sample_name)
{
   const auto cached = _loudness_envelopes.find(sample_name);
   if (cached != _loudness_envelopes.end())
   {
      return cached->second.empty() ? nullptr : &cached->second;
   }

   const auto sample_data = _backend->getSampleData(sample_name);
   if (!sample_data.has_value() || sample_data->_samples == nullptr || sample_data->_sample_count == 0 ||
       sample_data->_channel_count == 0 || sample_data->_sample_rate == 0)
   {
      // remember the failure, too, so a broken sample is not analyzed over and over again
      _loudness_envelopes[sample_name] = {};
      return nullptr;
   }

   auto& envelope = _loudness_envelopes[sample_name];
   envelope = computeLoudnessEnvelope(sample_data.value());

   return envelope.empty() ? nullptr : &envelope;
}

std::optional<float> Audio::getSampleLoudness(int32_t thread)
{
   std::lock_guard<std::mutex> guard(_mutex);

   if (_stopped)
   {
      return std::nullopt;
   }

   auto& sound_thread = _sound_threads[thread];
   if (!sound_thread._sound || !_backend->isActive(*sound_thread._sound))
   {
      return std::nullopt;
   }

   const auto* envelope = getLoudnessEnvelope(sound_thread._filename);
   if (envelope == nullptr)
   {
      return std::nullopt;
   }

   const auto playing_offset_s = sound_thread._sound->getPlayingOffset().asSeconds();
   const auto bucket = static_cast<size_t>(playing_offset_s / loudness_bucket_duration_s);

   return (*envelope)[std::min(bucket, envelope->size() - 1)];
}

void Audio::initializeSamples()
{
   addSample("coin.ogg");
   addSample("death.ogg");
   addSample("healthup.ogg");
   addSample("hurt.ogg");

   addSample("messagebox_open_01.ogg");
   addSample("messagebox_confirm.ogg");
   addSample("messagebox_cancel.ogg");

   addSample("arrow_hit_1.ogg");
   addSample("arrow_hit_2.ogg");

   addSample("powerup.ogg");
   addSample("splash.ogg");
   addSample("impact.ogg");
}

void Audio::debug()
{
   const auto stopped_thread_count = std::count_if(
      _sound_threads.begin(),
      _sound_threads.end(),
      [this](const auto& thread) { return thread._sound == nullptr || !_backend->isActive(*thread._sound); }
   );

   std::cout << stopped_thread_count << "/" << _sound_threads.size() << " are free" << std::endl;
}

void Audio::updateListenerPosition(const sf::Vector2f& pos)
{
   _backend->setListenerPosition(pos);
}

void Audio::adjustActiveSampleVolume()
{
   std::lock_guard<std::mutex> guard(_mutex);

   auto threads = _sound_threads |
                  std::views::filter([this](const auto& thread) { return thread._sound != nullptr && _backend->isActive(*thread._sound); });
   for (auto& thread : threads)
   {
      thread.setVolume(thread._play_info._volume);
   }
}

std::optional<int32_t> Audio::playSample(const PlayInfo& play_info)
{
   std::lock_guard<std::mutex> guard(_mutex);

   // debug();

   // find a free sample thread
   const auto& thread_it = std::find_if(
      _sound_threads.begin(),
      _sound_threads.end(),
      [this](const auto& thread) { return thread._sound == nullptr || !_backend->isActive(*thread._sound); }
   );

   if (thread_it == _sound_threads.cend())
   {
      Log::Error() << "no free thread to play: " << play_info._sample_name;
      return std::nullopt;
   }

   // check if we have the sample
   if (!_backend->hasSample(play_info._sample_name))
   {
      Log::Error() << "sample not found: " << play_info._sample_name;
      return std::nullopt;
   }

   const auto position = play_info._pos.value_or(sf::Vector3f{0.0f, 0.0f, 0.1f});

   auto prepared_sound = _backend->prepareSound(std::move(thread_it->_sound), play_info._sample_name);
   if (prepared_sound == nullptr)
   {
      return std::nullopt;
   }
   thread_it->_sound = std::move(prepared_sound);

   thread_it->_sound->setLooping(play_info._looped);
   thread_it->_sound->setPosition(position);
   thread_it->_sound->setMinDistance(10000.0f);
   thread_it->_sound->setAttenuation(0.0f);
   thread_it->_filename = play_info._sample_name;
   thread_it->_play_info = play_info;
   thread_it->setVolume(play_info._volume);
   thread_it->_sound->play();

   return static_cast<int32_t>(std::distance(_sound_threads.begin(), thread_it));
}

void Audio::stopSample(const std::string& name)
{
   std::lock_guard<std::mutex> guard(_mutex);
   if (_stopped)
   {
      return;
   }

   auto threads = _sound_threads | std::views::filter([name](const auto& thread) { return thread._filename == name; });
   for (auto& thread : threads)
   {
      if (thread._sound)
      {
         thread._sound->stop();
      }
   }
}

void Audio::stopSample(int32_t thread_index)
{
   std::lock_guard<std::mutex> guard(_mutex);
   if (_stopped)
   {
      return;
   }
   if (_sound_threads[thread_index]._sound)
   {
      _sound_threads[thread_index]._sound->stop();
   }
}

void Audio::setVolume(int32_t thread, float volume)
{
   std::lock_guard<std::mutex> guard(_mutex);
   _sound_threads[thread].setVolume(volume);
}

void Audio::setPosition(int32_t thread, const sf::Vector2f pos)
{
   std::lock_guard<std::mutex> guard(_mutex);
   _sound_threads[thread].setPosition(pos);
}

void Audio::SoundThread::setVolume(float volume)
{
   const auto master = (GameConfiguration::getInstance()._audio_volume_master * 0.01f);
   const auto sfx = (GameConfiguration::getInstance()._audio_volume_sfx) * 0.01f;
   _sound->setVolume(master * sfx * volume * Audio::getInstance()._backend->volumeScale());
}

void Audio::SoundThread::setPosition(const sf::Vector2f& pos)
{
   _sound->setPosition({pos.x, pos.y, 0.0f});
}
