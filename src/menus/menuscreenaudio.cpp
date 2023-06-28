#include "menuscreenaudio.h"

#include "game/audio.h"
#include "game/gameconfiguration.h"
#include "menu.h"
#include "menuaudio.h"

#define STEP_SIZE 10

MenuScreenAudio::MenuScreenAudio()
{
   setFilename("data/menus/audio.psd");
}

void MenuScreenAudio::up()
{
   auto next = static_cast<int32_t>(_selection);
   next--;
   if (next < 0)
   {
      next = static_cast<int32_t>(Selection::Count) - 1;
   }

   _selection = static_cast<Selection>(next);
   updateLayers();

   MenuAudio::play(MenuAudio::SoundEffect::ItemNavigate);
}

void MenuScreenAudio::down()
{
   auto next = static_cast<int32_t>(_selection);
   next++;
   if (next == static_cast<int32_t>(Selection::Count))
   {
      next = 0;
   }

   _selection = static_cast<Selection>(next);
   updateLayers();

   MenuAudio::play(MenuAudio::SoundEffect::ItemNavigate);
}

void MenuScreenAudio::select()
{
   MenuAudio::play(MenuAudio::SoundEffect::ItemSelect);
}

void MenuScreenAudio::back()
{
   Menu::getInstance()->show(Menu::MenuType::Options);
   MenuAudio::play(MenuAudio::SoundEffect::MenuBack);
}

void MenuScreenAudio::set(int32_t x)
{
   switch (_selection)
   {
      case Selection::Master:
      {
         GameConfiguration::getInstance()._audio_volume_master += x;
         break;
      }
      case Selection::Music:
      {
         GameConfiguration::getInstance()._audio_volume_music += x;
         break;
      }
      case Selection::SFX:
      {
         GameConfiguration::getInstance()._audio_volume_sfx += x;
         break;
      }
      case Selection::Count:
      {
         break;
      }
   }

   if (GameConfiguration::getInstance()._audio_volume_master > 100)
   {
      GameConfiguration::getInstance()._audio_volume_master = 100;
   }

   if (GameConfiguration::getInstance()._audio_volume_master < 0)
   {
      GameConfiguration::getInstance()._audio_volume_master = 0;
   }

   if (GameConfiguration::getInstance()._audio_volume_music > 100)
   {
      GameConfiguration::getInstance()._audio_volume_music = 100;
   }

   if (GameConfiguration::getInstance()._audio_volume_music < 0)
   {
      GameConfiguration::getInstance()._audio_volume_music = 0;
   }

   if (GameConfiguration::getInstance()._audio_volume_sfx > 100)
   {
      GameConfiguration::getInstance()._audio_volume_sfx = 100;
   }

   if (GameConfiguration::getInstance()._audio_volume_sfx < 0)
   {
      GameConfiguration::getInstance()._audio_volume_sfx = 0;
   }

   GameConfiguration::getInstance().serializeToFile();
   Audio::getInstance().initializeMusicVolume();

   updateLayers();

   MenuAudio::play(MenuAudio::SoundEffect::ItemTick);
}

void MenuScreenAudio::setDefaults()
{
   GameConfiguration::resetAudioDefaults();
   updateLayers();

   MenuAudio::play(MenuAudio::SoundEffect::Apply);
}

void MenuScreenAudio::keyboardKeyPressed(sf::Keyboard::Key key)
{
   if (key == sf::Keyboard::Up)
   {
      up();
   }

   else if (key == sf::Keyboard::Down)
   {
      down();
   }

   else if (key == sf::Keyboard::Return)
   {
      select();
   }

   else if (key == sf::Keyboard::Left)
   {
      set(-STEP_SIZE);
   }

   else if (key == sf::Keyboard::Right)
   {
      set(STEP_SIZE);
   }

   else if (key == sf::Keyboard::Escape)
   {
      back();
   }

   else if (key == sf::Keyboard::D)
   {
      setDefaults();
   }
}

void MenuScreenAudio::loadingFinished()
{
   updateLayers();
}

void MenuScreenAudio::showEvent()
{
   updateLayers();
}

void MenuScreenAudio::updateLayers()
{
   auto master = _selection == Selection::Master;
   auto sfx = _selection == Selection::SFX;
   auto music = _selection == Selection::Music;

   _layers["defaults_xbox_0"]->_visible = isControllerUsed();
   _layers["defaults_xbox_1"]->_visible = false;
   _layers["back_xbox_0"]->_visible = isControllerUsed();
   _layers["back_xbox_1"]->_visible = false;

   _layers["defaults_pc_0"]->_visible = !isControllerUsed();
   _layers["defaults_pc_1"]->_visible = false;
   _layers["back_pc_0"]->_visible = !isControllerUsed();
   _layers["back_pc_1"]->_visible = false;

   _layers["sfxVolume_body_0"]->_visible = !sfx;
   _layers["sfxVolume_body_1"]->_visible = sfx;
   _layers["sfxVolume_text_0"]->_visible = !sfx;
   _layers["sfxVolume_text_1"]->_visible = sfx;
   _layers["sfxVolume_highlight"]->_visible = sfx;
   _layers["sfxVolume_help"]->_visible = sfx;
   _layers["sfxVolume_arrows"]->_visible = sfx;
   _layers["sfxVolume_h_0"]->_visible = !sfx;
   _layers["sfxVolume_h_1"]->_visible = sfx;

   // broken in dstar's psd
   // new: sfxVolume_value_0 .. sfxVolume_value_10
   // _layers["sfxVolume_value"]->_visible = true;

   _layers["mscVolume_body_0"]->_visible = !music;
   _layers["mscVolume_body_1"]->_visible = music;
   _layers["mscVolume_text_0"]->_visible = !music;
   _layers["mscVolume_text_1"]->_visible = music;
   _layers["mscVolume_highlight"]->_visible = music;
   _layers["mscVolume_help"]->_visible = music;
   _layers["mscVolume_arrows"]->_visible = music;
   _layers["mscVolume_h_0"]->_visible = !music;
   _layers["mscVolume_h_1"]->_visible = music;

   // sfxVolume_value_0 .. sfxVolume_value_10
   //
   // _layers["mscVolume_value"]->_visible = true;

   _layers["master_text_0"]->_visible = !master;
   _layers["master_text_1"]->_visible = master;
   _layers["master_body_1"]->_visible = master;
   _layers["master_highlight"]->_visible = master;
   _layers["master_help"]->_visible = master;
   _layers["master_arrows"]->_visible = master;
   _layers["master_h_0"]->_visible = !master;
   _layers["master_h_1"]->_visible = master;

   // same issue as above
   // _layers["master_value"]->_visible = true;
   //

   const auto master_volume = GameConfiguration::getInstance()._audio_volume_master;
   const auto sfx_volume = GameConfiguration::getInstance()._audio_volume_sfx;
   const auto music_volume = GameConfiguration::getInstance()._audio_volume_music;

   _layers["master_h_0"]->_sprite->setOrigin(50.0f - master_volume, 0.0f);
   _layers["sfxVolume_h_0"]->_sprite->setOrigin(50.0f - sfx_volume, 0.0f);
   _layers["mscVolume_h_0"]->_sprite->setOrigin(50.0f - music_volume, 0.0f);

   _layers["master_h_1"]->_sprite->setOrigin(50.0f - master_volume, 0.0f);
   _layers["sfxVolume_h_1"]->_sprite->setOrigin(50.0f - sfx_volume, 0.0f);
   _layers["mscVolume_h_1"]->_sprite->setOrigin(50.0f - music_volume, 0.0f);
}

/*
data/menus/audio.psd
    bg_temp
    audio-window-bg
    audio_window-main
    main_body_0
    header
*/
