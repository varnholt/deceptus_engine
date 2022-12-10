#include "ingamemenuaudio.h"

#include "audio.h"

InGameMenuAudio::InGameMenuAudio()
{
   Audio::getInstance().addSample("ingame_menu_menu_next.wav");
   Audio::getInstance().addSample("ingame_menu_menu_open.wav");
   Audio::getInstance().addSample("ingame_menu_menu_close.wav");
   Audio::getInstance().addSample("ingame_menu_item_select.wav");
   Audio::getInstance().addSample("ingame_menu_item_navigate.wav");
}

void InGameMenuAudio::play(SoundEffect effect)
{
   switch (effect)
   {
      case InGameMenuAudio::SoundEffect::MenuNext:
      {
         break;
      }
      case InGameMenuAudio::SoundEffect::MenuOpen:
      {
         break;
      }
      case InGameMenuAudio::SoundEffect::MenuClose:
      {
         break;
      }
      case InGameMenuAudio::SoundEffect::ItemSelect:
      {
         break;
      }
      case InGameMenuAudio::SoundEffect::ItemNavigate:
      {
         break;
      }
   }
}
