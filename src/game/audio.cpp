#include "audio.h"

#include "gameconfiguration.h"

#include <string>
#include <iostream>
#include <filesystem>


namespace
{
const std::string sfx_root = "data/sounds/";
const std::string path = "data/music";
}

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
   initializeTracks();
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
   if (_sounds.find(sample) == _sounds.end())
   {
      auto loader = [](const std::string& fileName) -> sf::SoundBuffer
      {
         sf::SoundBuffer buf;
         buf.loadFromFile(sfx_root + fileName);
         return buf;
      };

      _sounds[sample] = loader(sample);
   }
}



//-----------------------------------------------------------------------------
void Audio::initializeSamples()
{
   addSample("coin.wav");
   addSample("death.wav");
   addSample("footstep.wav");
   addSample("healthup.wav");
   addSample("hurt.wav");

   addSample("player_grunt_01.wav");
   addSample("player_grunt_02.wav");
   addSample("player_jump_land.wav");
   addSample("player_jump_liftoff.wav");
   addSample("player_footstep_stone_01.ogg");

   addSample("powerup.wav");
   addSample("splash.wav");
   addSample("impact.wav");
}


//-----------------------------------------------------------------------------
void Audio::initializeTracks()
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
void Audio::playSample(const std::string& sample, float volume)
{
   const auto& thread_it = std::find_if(
      _threads.begin(),
      _threads.end(),
      [](const auto& thread){return thread.getStatus() == sf::Sound::Stopped;}
   );

   if (thread_it == _threads.end())
   {
      return;
   }

   auto it = _sounds.find(sample);

   if (it != _sounds.end())
   {
      thread_it->setBuffer(it->second);

      const auto master = (GameConfiguration::getInstance()._audio_volume_master * 0.01f);
      const auto sfx = (GameConfiguration::getInstance()._audio_volume_sfx) * 0.01f;

      thread_it->setVolume(master * sfx * volume * 100.0f);
      thread_it->play();
   }
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

