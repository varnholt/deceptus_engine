#include "audio.h"

#include "framework/tools/log.h"
#include "game/config/gameconfiguration.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <ranges>
#include <string>

namespace
{
const std::string sfx_path = "data/sounds/";
const std::string music_path = "data/music";
}  // namespace

/*!
 * \brief Audio::Audio
 * \param parent
 */
Audio::Audio()
{
#ifdef __EMSCRIPTEN__
   auto handle = sf::AudioContext::getDefaultPlaybackDeviceHandle();
   if (handle.hasValue())
   {
      _playback_device = std::make_unique<sf::PlaybackDevice>(*handle);
   }
#endif
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

#ifdef __EMSCRIPTEN__
sf::base::Optional<sf::SoundBuffer> Audio::loadFile(const std::string& filename)
{
   const std::string full_path = sfx_path + filename;
   if (!std::filesystem::exists(full_path))
   {
      Log::Error() << "audio file does not exist: " << filename;
      return sf::base::nullOpt;
   }

   auto start_time = std::chrono::high_resolution_clock::now();
   auto buffer = sf::SoundBuffer::loadFromFile(sf::Path{full_path});
   auto end_time = std::chrono::high_resolution_clock::now();

   auto load_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

   if (!buffer.hasValue())
   {
      Log::Error() << "unable to load file: " << filename;
      return sf::base::nullOpt;
   }
   else if (load_duration.count() >= 100)
   {
      Log::Info() << "Audio load time for " << filename << ": " << load_duration.count() << " ms (Main thread - may cause hiccups)";
   }

   if ((*buffer).getChannelCount() < 2)
   {
      Log::Warning() << filename << " seems to be mono :(";
   }

   return buffer;
};
#else
std::shared_ptr<sf::SoundBuffer> Audio::loadFile(const std::string& filename)
{
   // Check if the file exists before attempting to load
   const std::string full_path = sfx_path + filename;
   if (!std::filesystem::exists(full_path))
   {
      Log::Error() << "audio file does not exist: " << filename;
      return nullptr;
   }

   auto buffer = std::make_shared<sf::SoundBuffer>();

   // Time the audio file loading operation
   auto start_time = std::chrono::high_resolution_clock::now();
   bool success = buffer->loadFromFile(full_path);
   auto end_time = std::chrono::high_resolution_clock::now();

   auto load_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

   if (!success)
   {
      Log::Error() << "unable to load file: " << filename;
      return nullptr;
   }
   else if (load_duration.count() >= 100)
   {
      Log::Info() << "Audio load time for " << filename << ": " << load_duration.count() << " ms (Main thread - may cause hiccups)";
   }

   if (buffer->getChannelCount() < 2)
   {
      Log::Warning() << filename << " seems to be mono :(";
   }

   return buffer;
};
#endif

void Audio::addSample(const std::string& sample)
{
   std::lock_guard<std::mutex> lock(_mutex);

   if (_sound_buffers.find(sample) != _sound_buffers.end())
   {
      return;
   }

   auto buffer = loadFile(sample);

#ifdef __EMSCRIPTEN__
   if (buffer.hasValue())
   {
      _sound_buffers.emplace(sample, std::move(*buffer));
   }
#else
   if (buffer != nullptr)
   {
      _sound_buffers[sample] = buffer;
   }
#endif
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
#ifdef __EMSCRIPTEN__
      [](const auto& thread) { return thread._sound == nullptr || !thread._sound->isPlaying(); }
#else
      [](const auto& thread) { return thread._sound->getStatus() == sf::Sound::Status::Stopped; }
#endif
   );

   std::cout << stopped_thread_count << "/" << _sound_threads.size() << " are free" << std::endl;
}

void Audio::updateListenerPosition(const sf::Vector2f& pos)
{
#ifdef __EMSCRIPTEN__
   if (!_playback_device)
   {
      return;
   }
   sf::Listener listener;
   listener.position = {pos.x, pos.y, 0.0f};
   _playback_device->applyListener(listener);
#else
   sf::Listener::setPosition({pos.x, pos.y, 0.0f});
#endif
}

void Audio::adjustActiveSampleVolume()
{
   std::lock_guard<std::mutex> guard(_mutex);

   auto threads =
#ifdef __EMSCRIPTEN__
      _sound_threads | std::views::filter([](const auto& thread) { return thread._sound != nullptr && thread._sound->isPlaying(); });
#else
      _sound_threads | std::views::filter([](const auto& thread)
                                          { return thread._sound != nullptr && thread._sound->getStatus() != sf::Sound::Status::Stopped; });
#endif
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
#ifdef __EMSCRIPTEN__
      [](const auto& thread) { return thread._sound == nullptr || !thread._sound->isPlaying(); }
#else
      [](const auto& thread) { return thread._sound == nullptr || thread._sound->getStatus() == sf::Sound::Status::Stopped; }
#endif
   );

   if (thread_it == _sound_threads.cend())
   {
      Log::Error() << "no free thread to play: " << play_info._sample_name;
      return std::nullopt;
   }

#ifdef __EMSCRIPTEN__
   if (!_playback_device)
   {
      return std::nullopt;
   }
#endif

   // check if we have the sample
   const auto it = _sound_buffers.find(play_info._sample_name);
   if (it == _sound_buffers.cend())
   {
      Log::Error() << "sample not found: " << play_info._sample_name;
      return std::nullopt;
   }

#ifdef __EMSCRIPTEN__
   const auto position = play_info._pos.value_or(sf::Vec3f{0.0f, 0.0f, 0.1f});

   thread_it->_sound = std::make_unique<sf::Sound>(*_playback_device, it->second);
#else
   const auto position = play_info._pos.value_or(sf::Vector3f{0.0f, 0.0f, 0.1f});

   if (thread_it->_sound == nullptr)
   {
      thread_it->_sound = std::make_unique<sf::Sound>(*it->second);
   }
   else
   {
      thread_it->_sound->setBuffer(*it->second);
   }
#endif

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
#ifdef __EMSCRIPTEN__
   _sound->setVolume(master * sfx * volume);
#else
   _sound->setVolume(master * sfx * volume * 100.0f);
#endif
}

void Audio::SoundThread::setPosition(const sf::Vector2f& pos)
{
   _sound->setPosition({pos.x, pos.y, 0.0f});
}
