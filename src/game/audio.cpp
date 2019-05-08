#include "audio.h"

#include "gameconfiguration.h"


Audio* Audio::sInstance = nullptr;

namespace
{
static const std::string SFX_ROOT = "data/sounds/";
}

//-----------------------------------------------------------------------------
/*!
 * \brief Audio::Audio
 * \param parent
 */
Audio::Audio()
{
   sInstance = this;
}


//-----------------------------------------------------------------------------
/*!
 * \brief Audio::getInstance
 * \return
 */
Audio *Audio::getInstance()
{
   if (sInstance == nullptr)
   {
      new Audio();
      sInstance->initialize();
   }

   return sInstance;
}


//-----------------------------------------------------------------------------
void Audio::addSample(const std::string& sample)
{
   auto loader = [](const std::string& fileName) -> sf::SoundBuffer {
      sf::SoundBuffer buf;
      buf.loadFromFile(SFX_ROOT + fileName);
      return buf;
   };

   if (mSounds.find(sample) == mSounds.end())
   {
      mSounds[sample] = loader(sample);
   }
}



//-----------------------------------------------------------------------------
void Audio::initialize()
{
   addSample("coin.wav");
   addSample("death.wav");
   addSample("footstep.wav");
   addSample("healthup.wav");
   addSample("hurt.wav");
   addSample("jump.wav");
   addSample("powerup.wav");
   addSample("splash.wav");
   addSample("impact.wav");
}


//-----------------------------------------------------------------------------
void Audio::initializeMusicVolume()
{
   const auto master = (GameConfiguration::getInstance().mAudioVolumeMaster * 0.01f);
   const auto music = (GameConfiguration::getInstance().mAudioVolumeMusic * 0.01f);
   Audio::getInstance()->getMusic().setVolume(master * music);
}


//-----------------------------------------------------------------------------
void Audio::playSample(const std::string& sample, float volume)
{
   sf::Sound* sound = nullptr;
   for (int i = 0; i < 10; i++)
   {
      if (mThreads[i].getStatus() == sf::Sound::Stopped)
      {
         sound = &mThreads[i];
         break;
      }
   }

   if (sound == nullptr)
   {
      return;
   }

   auto it = mSounds.find(sample);

   if (it != mSounds.end())
   {
      sound->setBuffer(it->second);

      const auto master = (GameConfiguration::getInstance().mAudioVolumeMaster * 0.01f);
      const auto sfx = (GameConfiguration::getInstance().mAudioVolumeSfx * 0.01f);

      sound->setVolume(master * sfx * volume);
      sound->play();
   }
}


//-----------------------------------------------------------------------------
sf::Music& Audio::getMusic() const
{
   return mMusic;
}

