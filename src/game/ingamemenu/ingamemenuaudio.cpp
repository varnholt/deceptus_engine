#include "ingamemenuaudio.h"

#include "game/audio/audio.h"

InGameMenuAudio::InGameMenuAudio()
{
   Audio::getInstance().addSample("ingame_menu_menu_next.ogg");
   Audio::getInstance().addSample("ingame_menu_menu_open.ogg");
   Audio::getInstance().addSample("ingame_menu_menu_close.ogg");
   Audio::getInstance().addSample("ingame_menu_item_select.ogg");
   Audio::getInstance().addSample("ingame_menu_item_navigate.ogg");
}

void InGameMenuAudio::play(SoundEffect effect)
{
   switch (effect)
   {
      case InGameMenuAudio::SoundEffect::MenuNext:
      {
         Audio::getInstance().playSample({"ingame_menu_menu_next.ogg"});
         break;
      }
      case InGameMenuAudio::SoundEffect::MenuOpen:
      {
         Audio::getInstance().playSample({"ingame_menu_menu_open.ogg"});
         break;
      }
      case InGameMenuAudio::SoundEffect::MenuClose:
      {
         Audio::getInstance().playSample({"ingame_menu_menu_close.ogg"});
         break;
      }
      case InGameMenuAudio::SoundEffect::ItemSelect:
      {
         Audio::getInstance().playSample({"ingame_menu_item_select.ogg"});
         break;
      }
      case InGameMenuAudio::SoundEffect::ItemNavigate:
      {
         Audio::getInstance().playSample({"ingame_menu_item_navigate.ogg"});
         break;
      }
   }
}
