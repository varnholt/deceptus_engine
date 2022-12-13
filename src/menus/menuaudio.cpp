#include "menuaudio.h"

#include "audio.h"

void MenuAudio::play(SoundEffect effect)
{
   switch (effect)
   {
      case SoundEffect::Apply:
      {
         Audio::getInstance().playSample("menu_apply_01.wav");
         break;
      }
      case SoundEffect::MenuBack:
      {
         Audio::getInstance().playSample("menu_back_01.wav");
         break;
      }
      case SoundEffect::ItemTick:
      {
         Audio::getInstance().playSample("menu_tick_01.wav");
         break;
      }
      case SoundEffect::ItemNavigate:
      {
         Audio::getInstance().playSample("menu_click_01.wav");
         break;
      }
      case SoundEffect::ItemSelect:
      {
         Audio::getInstance().playSample("menu_select_01.wav");
         break;
      }
   }
}

void MenuAudio::initialize()
{
   Audio::getInstance().addSample("menu_apply_01.wav");
   Audio::getInstance().addSample("menu_back_01.wav");
   Audio::getInstance().addSample("menu_click_01.wav");
   Audio::getInstance().addSample("menu_select_01.wav");
   Audio::getInstance().addSample("menu_tick_01.wav");
}
