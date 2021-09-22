#pragma once

#include <SFML/Audio.hpp>
#include <array>
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

   std::map<std::string, sf::SoundBuffer> _sounds;
   std::array<sf::Sound, 10> _threads;

   mutable sf::Music _music;
   std::vector<Track> _tracks;
   uint32_t _current_index = 999;

   static Audio* __instance;
};

