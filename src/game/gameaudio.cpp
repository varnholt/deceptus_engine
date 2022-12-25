#include "gameaudio.h"

#include "audio.h"

void GameAudio::initialize()
{
   Audio::getInstance().addSample("gamestate_pause.ogg");
   Audio::getInstance().addSample("gamestate_resume.ogg");
}

void GameAudio::play(SoundEffect effect)
{
   switch (effect)
   {
      case GameAudio::SoundEffect::GameStatePause:
      {
         Audio::getInstance().playSample({"gamestate_pause.ogg"});
         break;
      }
      case GameAudio::SoundEffect::GameStateResume:
      {
         Audio::getInstance().playSample({"gamestate_resume.ogg"});
         break;
      }
   }
}

GameAudio& GameAudio::getInstance()
{
   static GameAudio __game_audio;
   return __game_audio;
}
