#pragma once

#include <SFML/Audio.hpp>
#include <map>

class Audio
{

public:

   struct Track {
       std::string mFilename;
   };

   static Audio* getInstance();

   void initializeMusicVolume();

   void addSample(const std::string& sample);
   void playSample(const std::string& name, float volume = 30.0f);
   void updateMusic();

   sf::Music& getMusic() const;


private:

   Audio();

   void initializeSamples();
   void initializeTracks();

   std::map<std::string, sf::SoundBuffer> mSounds;
   sf::Sound mThreads[10];

   mutable sf::Music mMusic;
   std::vector<Track> mTracks;
   uint32_t mCurrentTrackIndex = 999;

   static Audio* sInstance;
};

