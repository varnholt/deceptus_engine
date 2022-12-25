#pragma once

#include <SFML/Audio.hpp>
#include <array>
#include <functional>
#include <map>
#include <mutex>
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
   Audio();
   virtual ~Audio();

   struct Track
   {
      std::string _filename;
   };

   struct SoundThread
   {
      std::string _filename;
      sf::Sound _sound;

      void setVolume(float volume);
   };

   struct PlayInfo
   {
      std::string _sample_name;
      float _volume = 1.0f;
      bool _looped = false;
      std::optional<sf::Vector2f> _pos;
   };

   static Audio& getInstance();

   void initializeMusicVolume();

   void addSample(const std::string& sample);
   std::optional<int32_t> playSample(const PlayInfo& play_info);
   void stopSample(const std::string& name);
   void stopSample(int32_t thread);
   void setVolume(int32_t thread, float volume);

   void updateMusic();

   sf::Music& getMusic() const;

   using ShutdownCallback = std::function<void()>;
   void addShutdownCallback(const ShutdownCallback& shutdown_callback);

private:

   void initializeSamples();
   void initializeMusic();
   std::shared_ptr<sf::SoundBuffer> loadFile(const std::string& filename);
   void debug();

   std::mutex _mutex;
   std::map<std::string, std::shared_ptr<sf::SoundBuffer>> _sound_buffers;
   std::array<SoundThread, 50> _sound_threads;

   mutable sf::Music _music;
   std::vector<Track> _tracks;
   uint32_t _current_index = 999;

   std::vector<ShutdownCallback> _shutdown_callbacks;
};
