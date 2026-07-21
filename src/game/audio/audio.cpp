#include "audio.h"

#include "framework/tools/log.h"
#include "game/config/gameconfiguration.h"

#include <algorithm>
#include <iostream>
#include <ranges>
#include <string>

namespace
{
const std::string music_path = "data/music";
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

void Audio::initializeSamples()
{
   addSample("coin.wav");
   addSample("death.wav");
   addSample("healthup.wav");
   addSample("hurt.wav");

   addSample("messagebox_open_01.wav");
   addSample("messagebox_confirm.wav");
   addSample("messagebox_cancel.wav");

   addSample("arrow_hit_1.wav");
   addSample("arrow_hit_2.wav");

   addSample("powerup.wav");
   addSample("splash.wav");
   addSample("impact.wav");
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
