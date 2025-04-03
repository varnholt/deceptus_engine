#include "audio.h"

#include "framework/tools/log.h"
#include "game/config/gameconfiguration.h"

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

std::shared_ptr<sf::SoundBuffer> Audio::loadFile(const std::string& filename)
{
   auto buf = std::make_shared<sf::SoundBuffer>();
   if (!buf->loadFromFile(sfx_path + filename))
   {
      Log::Error() << "unable to load file: " << filename;
   }

   if (buf->getChannelCount() < 2)
   {
      Log::Warning() << filename << " seems to be mono :(";
   }
   return buf;
};

void Audio::addSample(const std::string& sample)
{
   if (_sound_buffers.find(sample) != _sound_buffers.end())
   {
      return;
   }

   _sound_buffers[sample] = loadFile(sample);
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
      _sound_threads.begin(), _sound_threads.end(), [](const auto& thread) { return thread._sound.getStatus() == sf::Sound::Stopped; }
   );

   std::cout << stopped_thread_count << "/" << _sound_threads.size() << " are free" << std::endl;
}

Audio::MusicPlayer& Audio::getMusicPlayer()
{
   return _music_player;
}

void Audio::adjustActiveSampleVolume()
{
   std::lock_guard<std::mutex> guard(_mutex);

   auto threads = _sound_threads | std::views::filter([](const auto& thread) { return thread._sound.getStatus() != sf::Sound::Stopped; });
   for (auto& thread : threads)
   {
      std::cout << thread._play_info._volume << std::endl;
      thread.setVolume(thread._play_info._volume);
   }
}

std::optional<int32_t> Audio::playSample(const PlayInfo& play_info)
{
   std::lock_guard<std::mutex> guard(_mutex);

   // debug();

   // find a free sample thread
   const auto& thread_it = std::find_if(
      _sound_threads.begin(), _sound_threads.end(), [](const auto& thread) { return thread._sound.getStatus() == sf::Sound::Stopped; }
   );

   if (thread_it == _sound_threads.cend())
   {
      Log::Error() << "no free thread to play: " << play_info._sample_name;
      return std::nullopt;
   }

   // check if we have the sample
   const auto it = _sound_buffers.find(play_info._sample_name);
   if (it == _sound_buffers.cend())
   {
      Log::Error() << "sample not found: " << play_info._sample_name;
      return std::nullopt;
   }

   thread_it->_sound.setBuffer(*it->second);
   thread_it->_filename = play_info._sample_name;
   thread_it->setVolume(play_info._volume);
   thread_it->_sound.setLoop(play_info._looped);
   thread_it->_play_info = play_info;

   if (play_info._pos.has_value())
   {
      thread_it->_sound.setPosition(play_info._pos->x, play_info._pos->y, 0.1f);
   }
   else
   {
      // https://github.com/SFML/SFML/issues/2319
      // for mono sounds, the listener position must be != (0, 0, 0)
      thread_it->_sound.setPosition(0.0f, 0.0f, 0.1f);
   }

   thread_it->_sound.play();

   return static_cast<int32_t>(std::distance(_sound_threads.begin(), thread_it));
}

void Audio::stopSample(const std::string& name)
{
   std::lock_guard<std::mutex> guard(_mutex);

   auto threads = _sound_threads | std::views::filter([name](const auto& thread) { return thread._filename == name; });
   for (auto& thread : threads)
   {
      thread._sound.stop();
   }
}

void Audio::stopSample(int32_t thread)
{
   std::lock_guard<std::mutex> guard(_mutex);

   _sound_threads[thread]._sound.stop();
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
   _sound.setVolume(master * sfx * volume * 100.0f);
}

void Audio::SoundThread::setPosition(const sf::Vector2f& pos)
{
   _sound.setPosition(pos.x, pos.y, 0.0f);
}

void Audio::MusicPlayer::update(const sf::Time& dt)
{
   const auto dt_ms = std::chrono::milliseconds(dt.asMilliseconds());

   if (_is_crossfading)
   {
      _crossfade_elapsed += dt_ms;
      const auto& total = _crossfade_duration;

      const double t = std::min(1.0, static_cast<double>(_crossfade_elapsed.count()) / total.count());

      _next->setVolume(static_cast<float>(volume() * t));
      _current->setVolume(static_cast<float>(volume() * (1.0 - t)));

      if (_crossfade_elapsed >= total)
      {
         _current->stop();
         _current->setVolume(volume());
         _current = _next;
         _is_crossfading = false;
         _pending_request.reset();
      }

      return;
   }

   if (!_pending_request.has_value())
   {
      return;
   }

   const auto& request = _pending_request.value();

   switch (request.transition)
   {
      case TransitionType::ImmediateSwitch:
      {
         beginTransition(request);
         _pending_request.reset();
         break;
      }
      case TransitionType::LetCurrentFinish:
      {
         if (!_current || _current->getStatus() != sf::Music::Playing)
         {
            beginTransition(request);
            _pending_request.reset();
         }
         break;
      }
      case TransitionType::Crossfade:
      {
         beginTransition(request);
         _is_crossfading = true;
         _crossfade_elapsed = std::chrono::milliseconds{0};
         _crossfade_duration = request.duration;
         break;
      }
      case TransitionType::FadeOutThenNew:
      {
         // not implemented yet
         break;
      }
   }
}

void Audio::MusicPlayer::beginTransition(const TrackRequest& request)
{
   _using_a = !_using_a;
   _next = _using_a ? &_music_a : &_music_b;
   _current = _using_a ? &_music_b : &_music_a;

   if (!_next->openFromFile(request.filename))
   {
      // optionally handle error
      return;
   }

   if (request.transition == TransitionType::Crossfade)
   {
      _next->setVolume(0.f);
   }
   else
   {
      _next->setVolume(100.f);
      if (_current)
      {
         _current->stop();
      }
   }

   _next->play();
}

float Audio::MusicPlayer::volume() const
{
   const auto& config = GameConfiguration::getInstance();
   const auto master = config._audio_volume_master * 0.01f;
   const auto music = config._audio_volume_music;
   return master * music;
}
