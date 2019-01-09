#include "audio.h"


Audio* Audio::sInstance = nullptr;

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
void Audio::initialize()
{
   auto loader = [](const std::string& fileName) -> sf::SoundBuffer {
      sf::SoundBuffer buf;
      buf.loadFromFile(fileName);
      return buf;
   };

   mSounds.insert(std::pair<Sample, sf::SoundBuffer>(SampleCoin, loader("data/sounds/coin.wav")));
   mSounds.insert(std::pair<Sample, sf::SoundBuffer>(SampleDeath, loader("data/sounds/death.wav")));
   mSounds.insert(std::pair<Sample, sf::SoundBuffer>(SampleFootstep, loader("data/sounds/footstep.wav")));
   mSounds.insert(std::pair<Sample, sf::SoundBuffer>(SampleHealthUp, loader("data/sounds/healthup.wav")));
   mSounds.insert(std::pair<Sample, sf::SoundBuffer>(SampleHurt, loader("data/sounds/hurt.wav")));
   mSounds.insert(std::pair<Sample, sf::SoundBuffer>(SampleJump, loader("data/sounds/jump.wav")));
   mSounds.insert(std::pair<Sample, sf::SoundBuffer>(SamplePowerUp, loader("data/sounds/powerup.wav")));
   mSounds.insert(std::pair<Sample, sf::SoundBuffer>(SampleSplash, loader("data/sounds/splash.wav")));
   mSounds.insert(std::pair<Sample, sf::SoundBuffer>(SampleImpact, loader("data/sounds/impact.wav")));
   mSounds.insert(std::pair<Sample, sf::SoundBuffer>(SampleBoom, loader("data/sounds/boom.wav")));
}


//-----------------------------------------------------------------------------
void Audio::playSample(Audio::Sample sample, float volume)
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
      sound->setVolume(volume);
      sound->play();
   }
}

