#ifndef AUDIO_H
#define AUDIO_H

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

   void playSample(Sample, float volume = 30.0f);


protected:

   Audio();

   std::map<Sample, sf::SoundBuffer> mSounds;
   sf::Sound mThreads[10];

   static Audio* sInstance;
};

#endif // AUDIO_H
