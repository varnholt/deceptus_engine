#pragma once

#include <SFML/Audio.hpp>
#include <array>
#include <map>


/*! \brief The class Audio implements audio support
 *         It supports audio samples and music.
 *
 *  Samples are cached by calling addSample, and played by calling playSample.
 *  Music is loaded from disk and uses .ogg format. Once one track is consumed,
 *  updateMusic will select the next track to play.
 */
class Audio
{

public:

   struct Track
   {
      std::string _filename;
   };

   static Audio& getInstance();

   void initializeMusicVolume();

   void addSample(const std::string& sample);
   void playSample(const std::string& name, float volume = 1.0f);
   void pauseSample(const std::string& name);
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
};

