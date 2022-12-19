#pragma once

#include <SFML/Audio.hpp>
#include <array>
#include <map>
#include <optional>

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

   struct SampleThread
   {
      std::string _filename;
      sf::Sound _sound;

      void setVolume(float volume);
   };

   static Audio& getInstance();

   void initializeMusicVolume();

   void addSample(const std::string& sample);
   std::optional<int32_t> playSample(const std::string& name, float volume = 1.0f, bool looped = false);
   void stopSample(const std::string& name);
   void stopSample(int32_t thread);
   void setVolume(int32_t thread, float volume);

   void updateMusic();

   sf::Music& getMusic() const;


private:

   Audio();

   void initializeSamples();
   void initializeMusic();

   std::map<std::string, sf::SoundBuffer> _sounds;
   std::array<SampleThread, 50> _threads;

   mutable sf::Music _music;
   std::vector<Track> _tracks;
   uint32_t _current_index = 999;
};

