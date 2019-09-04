#pragma once

#include <SFML/Audio.hpp>
#include <map>

class Audio
{

public:

   static Audio* getInstance();

   void initializeSamples();
   void initializeMusicVolume();

   void addSample(const std::string& sample);
   void playSample(const std::string& name, float volume = 30.0f);

   sf::Music& getMusic() const;


protected:

   Audio();

   std::map<std::string, sf::SoundBuffer> mSounds;
   sf::Sound mThreads[10];
   mutable sf::Music mMusic;

   static Audio* sInstance;
};

