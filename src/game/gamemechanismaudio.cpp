#include "gamemechanismaudio.h"

#include "audio.h"

void GameMechanismAudio::initialize()
{
   Audio::getInstance().addSample("mechanism_bouncer.wav");
}

void GameMechanismAudio::play(Effect effect)
{
   switch (effect)
   {
      case GameMechanismAudio::Effect::BouncerJump:
      {
         break;
      }
   }
}

GameMechanismAudio& GameMechanismAudio::getInstance()
{
   static GameMechanismAudio __audio;
   return __audio;
}
