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
         Audio::getInstance().playSample("ingame_menu_menu_next.wav");
         break;
      }
      case InGameMenuAudio::SoundEffect::MenuOpen:
      {
         Audio::getInstance().playSample("ingame_menu_menu_open.wav");
         break;
      }
      case InGameMenuAudio::SoundEffect::MenuClose:
      {
         Audio::getInstance().playSample("ingame_menu_menu_close.wav");
         break;
      }
      case InGameMenuAudio::SoundEffect::ItemSelect:
      {
         Audio::getInstance().playSample("ingame_menu_item_select.wav");
         break;
      }
      case InGameMenuAudio::SoundEffect::ItemNavigate:
      {
         Audio::getInstance().playSample("ingame_menu_item_navigate.wav");
         break;
      }
   }
}
