#pragma once

#include <SFML/Audio.hpp>
#include <map>

class Audio
{

public:

   enum Sample
   {
      SampleCoin,
      SampleDeath,
      SampleFootstep,
      SampleJump,
      SampleHealthUp,
      SampleHurt,
      SampleImpact,
      SamplePowerUp,
      SampleSplash,
      SampleBoom
   };

   static Audio* getInstance();

   void initialize();

   void initializeMusicVolume();

   void playSample(Sample, float volume = 30.0f);

   sf::Music& getMusic() const;


protected:

   Audio();

   std::map<Sample, sf::SoundBuffer> mSounds;
   sf::Sound mThreads[10];
   mutable sf::Music mMusic;

   static Audio* sInstance;
};

