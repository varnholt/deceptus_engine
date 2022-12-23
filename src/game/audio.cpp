#include "audio.h"

#include "framework/tools/log.h"
#include "gameconfiguration.h"

#include <filesystem>
#include <iostream>
#include <ranges>
#include <string>

namespace
{
const std::string sfx_root = "data/sounds/";
const std::string path = "data/music";
}  // namespace

/*

ui_back_cancel
ui_menu_open
ui_menu_close
ui_menu_select
ui_menu_accept
ui_menu_item_navigation
ui_letter_sound
ui_message_box_close
ui_message_box_open
ui_state_pause
ui_state_resume

 */

//-----------------------------------------------------------------------------
/*!
 * \brief Audio::Audio
 * \param parent
 */
Audio::Audio()
{
   initializeMusicVolume();
   initializeSamples();
   initializeMusic();
}

//-----------------------------------------------------------------------------
/*!
 * \brief Audio::getInstance
 * \return
 */
Audio& Audio::getInstance()
{
   static Audio __instance;
   return __instance;
}

//-----------------------------------------------------------------------------
void Audio::addSample(const std::string& sample)
{
   if (_sounds.find(sample) != _sounds.end())
   {
      return;
   }

   auto loader = [](const std::string& fileName) -> sf::SoundBuffer
   {
      sf::SoundBuffer buf;
      if (!buf.loadFromFile(sfx_root + fileName))
      {
         Log::Error() << "unable to load file: " << fileName;
      }
      return buf;
   };

   _sounds[sample] = loader(sample);
}

//-----------------------------------------------------------------------------
void Audio::initializeSamples()
{
   addSample("coin.wav");
   addSample("death.wav");
   addSample("footstep.wav");
   addSample("healthup.wav");
   addSample("hurt.wav");

   addSample("messagebox_open_01.wav");
   addSample("messagebox_confirm.wav");
   addSample("messagebox_cancel.wav");

   addSample("arrow_hit_1.wav");
   addSample("arrow_hit_2.wav");

   // make separate audio interface for player
   addSample("player_doublejump_01.mp3");
   addSample("player_doublejump_02.mp3");
   addSample("player_grunt_01.wav");
   addSample("player_grunt_02.wav");
   addSample("player_jump_land.wav");
   addSample("player_jump_liftoff.wav");
   addSample("player_kneel_01.wav");
   addSample("player_footstep_stone_l.wav");
   addSample("player_footstep_stone_r.wav");
   addSample("player_spawn_01.wav");
   addSample("player_sword_kneeling_01.wav");
   addSample("player_sword_kneeling_02.wav");
   addSample("player_sword_kneeling_03.wav");
   addSample("player_sword_kneeling_04.wav");
   addSample("player_sword_standing_01.wav");
   addSample("player_sword_standing_02.wav");
   addSample("player_sword_standing_03.wav");
   addSample("player_sword_standing_04.wav");
   addSample("player_sword_standing_05.wav");
   addSample("player_sword_standing_06.wav");
   addSample("player_sword_standing_07.wav");
   addSample("player_sword_standing_08.wav");
   addSample("player_sword_standing_09.wav");

   addSample("powerup.wav");
   addSample("splash.wav");
   addSample("impact.wav");
}

//-----------------------------------------------------------------------------
void Audio::initializeMusic()
{
   try
   {
      for (const auto& entry : std::filesystem::directory_iterator(path))
      {
         const auto path = entry.path().string();

         if (path.find(".ogg") != std::string::npos)
         {
            Track track;
            track._filename = path;
            _tracks.push_back(track);
         }

         // Log::Info() << entry.path();
      }
   }
   catch (std::exception&)
   {
   }
}

//-----------------------------------------------------------------------------
void Audio::initializeMusicVolume()
{
   const auto& config = GameConfiguration::getInstance();
   const auto master = config._audio_volume_master * 0.01f;
   const auto music = config._audio_volume_music;
   _music.setVolume(master * music);
}

//-----------------------------------------------------------------------------
std::optional<int32_t> Audio::playSample(const std::string& sample, float volume, bool looped)
{
   // find a free sample thread
   const auto& thread_it =
      std::find_if(_threads.begin(), _threads.end(), [](const auto& thread) { return thread._sound.getStatus() == sf::Sound::Stopped; });

   if (thread_it == _threads.cend())
   {
      return std::nullopt;
   }

   // check if we have the sample
   const auto it = _sounds.find(sample);

   if (it == _sounds.cend())
   {
      return std::nullopt;
   }

   thread_it->_sound.setBuffer(it->second);
   thread_it->_filename = sample;
   thread_it->setVolume(volume);
   thread_it->_sound.setLoop(looped);
   thread_it->_sound.play();

   return std::distance(_threads.begin(), thread_it);
}

//-----------------------------------------------------------------------------
void Audio::stopSample(const std::string& name)
{
   auto threads = _threads | std::views::filter([name](auto& thread) { return thread._filename == name; });
   for (auto& thread : threads)
   {
      thread._sound.stop();
   }
}

//-----------------------------------------------------------------------------
void Audio::stopSample(int32_t thread)
{
   _threads[thread]._sound.stop();
}

//-----------------------------------------------------------------------------
void Audio::setVolume(int32_t thread, float volume)
{
   _threads[thread].setVolume(volume);
}

//-----------------------------------------------------------------------------
void Audio::updateMusic()
{
   if (_tracks.empty())
   {
      return;
   }

   if (_music.getStatus() == sf::Music::Playing)
   {
      return;
   }

   _current_index++;

   if (_current_index >= _tracks.size())
   {
      _current_index = 0;
   }

   _music.openFromFile(_tracks[_current_index]._filename);
   _music.play();
}

//-----------------------------------------------------------------------------
sf::Music& Audio::getMusic() const
{
   return _music;
}

//-----------------------------------------------------------------------------
void Audio::SampleThread::setVolume(float volume)
{
   const auto master = (GameConfiguration::getInstance()._audio_volume_master * 0.01f);
   const auto sfx = (GameConfiguration::getInstance()._audio_volume_sfx) * 0.01f;
   _sound.setVolume(master * sfx * volume * 100.0f);
}
