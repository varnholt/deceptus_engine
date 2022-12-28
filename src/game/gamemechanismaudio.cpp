#include "gamemechanismaudio.h"

#include "audio.h"

void GameMechanismAudio::initialize()
{
   Audio::getInstance().addSample("mechanism_bouncer.wav");
   Audio::getInstance().addSample("mechanism_collapsing_platform_crumble.wav");
   Audio::getInstance().addSample("mechanism_switch_off.wav");
   Audio::getInstance().addSample("mechanism_switch_on.wav");
   Audio::getInstance().addSample("mechanism_spikeball_01.wav");
   Audio::getInstance().addSample("mechanism_spikeball_02.wav");
}

void GameMechanismAudio::play(Effect effect)
{
   switch (effect)
   {
      case GameMechanismAudio::Effect::BouncerJump:
      {
         Audio::getInstance().playSample({"mechanism_bouncer.wav"});
         break;
      }
      case GameMechanismAudio::Effect::CollapsingPlatformCrumble:
      {
         Audio::getInstance().playSample({"mechanism_collapsing_platform_crumble.wav"});
         break;
      }
      case GameMechanismAudio::Effect::LeverOn:
      {
         Audio::getInstance().playSample({"mechanism_switch_on.wav"});
         break;
      }
      case GameMechanismAudio::Effect::LeverOff:
      {
         Audio::getInstance().playSample({"mechanism_switch_off.wav"});
         break;
      }
   }
}

void GameMechanismAudio::stop(Effect effect)
{
   switch (effect)
   {
      case GameMechanismAudio::Effect::CollapsingPlatformCrumble:
      {
         Audio::getInstance().stopSample("mechanism_collapsing_platform_crumble.wav");
         break;
      }
      default:
      {
         break;
      }
   }
}
