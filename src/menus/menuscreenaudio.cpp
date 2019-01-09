#include "menuscreenaudio.h"

#include "menu.h"


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
   Menu::getInstance().show(Menu::MenuType::Options);
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

   else if (key == sf::Keyboard::Escape)
   {
      back();
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

   mLayers["defaults_xbox_0"]->mVisible = false;
   mLayers["defaults_xbox_1"]->mVisible = false;
   mLayers["back_xbox_0"]->mVisible = false;
   mLayers["back_xbox_1"]->mVisible = false;

   mLayers["defaults_pc_0"]->mVisible = true;
   mLayers["defaults_pc_1"]->mVisible = false;
   mLayers["back_pc_0"]->mVisible = true;
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
}


/*
data/menus/audio.psd
    bg_temp
    audio-window-bg
    audio_window-main
    main_body_0
    header
*/

