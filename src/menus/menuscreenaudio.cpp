#include "menuscreenaudio.h"

#include "menu.h"
#include "game/audio.h"
#include "game/gameconfiguration.h"

#define STEP_SIZE 10


MenuScreenAudio::MenuScreenAudio()
{
   setFilename("data/menus/audio.psd");
}


void MenuScreenAudio::up()
{
   auto next = static_cast<int32_t>(mSelection);
   next--;
   if (next < 0)
   {
      next = static_cast<int32_t>(Selection::Count) - 1;
   }

   mSelection = static_cast<Selection>(next);
   updateLayers();
}


void MenuScreenAudio::down()
{
   auto next = static_cast<int32_t>(mSelection);
   next++;
   if (next == static_cast<int32_t>(Selection::Count))
   {
      next = 0;
   }

   mSelection = static_cast<Selection>(next);
   updateLayers();
}


void MenuScreenAudio::select()
{
}


void MenuScreenAudio::back()
{
    Menu::getInstance()->show(Menu::MenuType::Options);
}


void MenuScreenAudio::set(int32_t x)
{
    switch (mSelection)
    {
        case Selection::Master:
            GameConfiguration::getInstance()._audio_volume_master += x;
            break;
        case Selection::Music:
            GameConfiguration::getInstance()._audio_volume_music += x;
            break;
        case Selection::SFX:
            GameConfiguration::getInstance()._audio_volume_sfx += x;
            break;
        case Selection::Count:
            break;
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
    Audio::getInstance()->initializeMusicVolume();

    updateLayers();
}


void MenuScreenAudio::setDefaults()
{
    GameConfiguration::resetAudioDefaults();
    updateLayers();
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
   auto master = mSelection == Selection::Master;
   auto sfx = mSelection == Selection::SFX;
   auto music = mSelection == Selection::Music;

   mLayers["defaults_xbox_0"]->_visible = isControllerUsed();
   mLayers["defaults_xbox_1"]->_visible = false;
   mLayers["back_xbox_0"]->_visible = isControllerUsed();
   mLayers["back_xbox_1"]->_visible = false;

   mLayers["defaults_pc_0"]->_visible = !isControllerUsed();
   mLayers["defaults_pc_1"]->_visible = false;
   mLayers["back_pc_0"]->_visible = !isControllerUsed();
   mLayers["back_pc_1"]->_visible = false;

   mLayers["sfxVolume_body_0"]->_visible = !sfx;
   mLayers["sfxVolume_body_1"]->_visible = sfx;
   mLayers["sfxVolume_text_0"]->_visible = !sfx;
   mLayers["sfxVolume_text_1"]->_visible = sfx;
   mLayers["sfxVolume_highlight"]->_visible = sfx;
   mLayers["sfxVolume_help"]->_visible = sfx;
   mLayers["sfxVolume_arrows"]->_visible = sfx;
   mLayers["sfxVolume_h_0"]->_visible = !sfx;
   mLayers["sfxVolume_h_1"]->_visible = sfx;
   mLayers["sfxVolume_value"]->_visible = true;

   mLayers["mscVolume_body_0"]->_visible = !music;
   mLayers["mscVolume_body_1"]->_visible = music;
   mLayers["mscVolume_text_0"]->_visible = !music;
   mLayers["mscVolume_text_1"]->_visible = music;
   mLayers["mscVolume_highlight"]->_visible = music;
   mLayers["mscVolume_help"]->_visible = music;
   mLayers["mscVolume_arrows"]->_visible = music;
   mLayers["mscVolume_h_0"]->_visible = !music;
   mLayers["mscVolume_h_1"]->_visible = music;
   mLayers["mscVolume_value"]->_visible = true;

   mLayers["master_text_0"]->_visible = !master;
   mLayers["master_text_1"]->_visible = master;
   mLayers["master_body_1"]->_visible = master;
   mLayers["master_highlight"]->_visible = master;
   mLayers["master_help"]->_visible = master;
   mLayers["master_arrows"]->_visible = master;
   mLayers["master_h_0"]->_visible = !master;
   mLayers["master_h_1"]->_visible = master;
   mLayers["master_value"]->_visible = true;

   const auto masterVolume = GameConfiguration::getInstance()._audio_volume_master;
   const auto sfxVolume = GameConfiguration::getInstance()._audio_volume_sfx;
   const auto musicVolume = GameConfiguration::getInstance()._audio_volume_music;

   mLayers["master_h_0"]->_sprite->setOrigin(50.0f - masterVolume, 0.0f);
   mLayers["sfxVolume_h_0"]->_sprite->setOrigin(50.0f - sfxVolume, 0.0f);
   mLayers["mscVolume_h_0"]->_sprite->setOrigin(50.0f - musicVolume, 0.0f);

   mLayers["master_h_1"]->_sprite->setOrigin(50.0f - masterVolume, 0.0f);
   mLayers["sfxVolume_h_1"]->_sprite->setOrigin(50.0f - sfxVolume, 0.0f);
   mLayers["mscVolume_h_1"]->_sprite->setOrigin(50.0f - musicVolume, 0.0f);
}


/*
data/menus/audio.psd
    bg_temp
    audio-window-bg
    audio_window-main
    main_body_0
    header
*/

