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
            GameConfiguration::getInstance().mAudioVolumeMaster += x;
            break;
        case Selection::Music:
            GameConfiguration::getInstance().mAudioVolumeMusic += x;
            break;
        case Selection::SFX:
            GameConfiguration::getInstance().mAudioVolumeSfx += x;
            break;
        case Selection::Count:
            break;
    }

    if (GameConfiguration::getInstance().mAudioVolumeMaster > 100)
    {
        GameConfiguration::getInstance().mAudioVolumeMaster = 100;
    }

    if (GameConfiguration::getInstance().mAudioVolumeMaster < 0)
    {
        GameConfiguration::getInstance().mAudioVolumeMaster = 0;
    }

    if (GameConfiguration::getInstance().mAudioVolumeMusic > 100)
    {
        GameConfiguration::getInstance().mAudioVolumeMusic = 100;
    }

    if (GameConfiguration::getInstance().mAudioVolumeMusic < 0)
    {
        GameConfiguration::getInstance().mAudioVolumeMusic = 0;
    }

    if (GameConfiguration::getInstance().mAudioVolumeSfx > 100)
    {
        GameConfiguration::getInstance().mAudioVolumeSfx = 100;
    }

    if (GameConfiguration::getInstance().mAudioVolumeSfx < 0)
    {
        GameConfiguration::getInstance().mAudioVolumeSfx = 0;
    }

    GameConfiguration::getInstance().serializeToFile();
    Audio::getInstance()->initializeMusicVolume();

    updateLayers();
}


void MenuScreenAudio::setDefaults()
{
    GameConfiguration defaults;
    GameConfiguration::getInstance().mAudioVolumeMaster = defaults.mAudioVolumeMaster;
    GameConfiguration::getInstance().mAudioVolumeMusic = defaults.mAudioVolumeMusic;
    GameConfiguration::getInstance().mAudioVolumeSfx = defaults.mAudioVolumeSfx;

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


void MenuScreenAudio::updateLayers()
{
   auto master = mSelection == Selection::Master;
   auto sfx = mSelection == Selection::SFX;
   auto music = mSelection == Selection::Music;

   mLayers["defaults_xbox_0"]->mVisible = isControllerUsed();
   mLayers["defaults_xbox_1"]->mVisible = false;
   mLayers["back_xbox_0"]->mVisible = isControllerUsed();
   mLayers["back_xbox_1"]->mVisible = false;

   mLayers["defaults_pc_0"]->mVisible = !isControllerUsed();
   mLayers["defaults_pc_1"]->mVisible = false;
   mLayers["back_pc_0"]->mVisible = !isControllerUsed();
   mLayers["back_pc_1"]->mVisible = false;

   mLayers["sfxVolume_body_0"]->mVisible = !sfx;
   mLayers["sfxVolume_body_1"]->mVisible = sfx;
   mLayers["sfxVolume_text_0"]->mVisible = !sfx;
   mLayers["sfxVolume_text_1"]->mVisible = sfx;
   mLayers["sfxVolume_highlight"]->mVisible = sfx;
   mLayers["sfxVolume_help"]->mVisible = sfx;
   mLayers["sfxVolume_arrows"]->mVisible = sfx;
   mLayers["sfxVolume_h"]->mVisible = true;
   mLayers["sfxVolume_value"]->mVisible = true;

   mLayers["mscVolume_body_0"]->mVisible = !music;
   mLayers["mscVolume_body_1"]->mVisible = music;
   mLayers["mscVolume_text_0"]->mVisible = !music;
   mLayers["mscVolume_text_1"]->mVisible = music;
   mLayers["mscVolume_highlight"]->mVisible = music;
   mLayers["mscVolume_help"]->mVisible = music;
   mLayers["mscVolume_arrows"]->mVisible = music;
   mLayers["mscVolume_h"]->mVisible = true;
   mLayers["mscVolume_value"]->mVisible = true;

   mLayers["master_text_0"]->mVisible = !master;
   mLayers["master_text_1"]->mVisible = master;
   mLayers["master_body_1"]->mVisible = master;
   mLayers["master_highlight"]->mVisible = master;
   mLayers["master_help"]->mVisible = master;
   mLayers["master_arrows"]->mVisible = master;
   mLayers["master_h"]->mVisible = true;
   mLayers["master_value"]->mVisible = true;

   const auto masterVolume = GameConfiguration::getInstance().mAudioVolumeMaster;
   const auto sfxVolume = GameConfiguration::getInstance().mAudioVolumeSfx;
   const auto musicVolume = GameConfiguration::getInstance().mAudioVolumeMusic;
   mLayers["master_h"]->mSprite->setOrigin(50 - masterVolume, 0);
   mLayers["sfxVolume_h"]->mSprite->setOrigin(50 - sfxVolume, 0);
   mLayers["mscVolume_h"]->mSprite->setOrigin(50 - musicVolume, 0);
}


/*
data/menus/audio.psd
    bg_temp
    audio-window-bg
    audio_window-main
    main_body_0
    header
*/

