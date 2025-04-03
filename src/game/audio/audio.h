#pragma once

#include <SFML/Audio.hpp>
#include <array>
#include <functional>
#include <map>
#include <memory>
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

   struct Track
   {
      std::string _filename;
   };

   struct PlayInfo
   {
      PlayInfo() = default;

      PlayInfo(const std::string& sample_name) : _sample_name(sample_name)
      {
      }

      PlayInfo(const std::string& sample_name, float volume) : _sample_name(sample_name), _volume(volume)
      {
      }

      PlayInfo(const std::string& sample_name, float volume, bool looped) : _sample_name(sample_name), _volume(volume), _looped(looped)
      {
      }

      std::string _sample_name;
      float _volume = 1.0f;
      bool _looped = false;
      std::optional<sf::Vector2f> _pos;
   };

   struct SoundThread
   {
      std::string _filename;
      sf::Sound _sound;
      PlayInfo _play_info;

      void setVolume(float volume);
      void setPosition(const sf::Vector2f& pos);
   };

   enum class TransitionType
   {
      LetCurrentFinish,
      Crossfade,
      ImmediateSwitch,
      FadeOutThenNew
   };

   struct TrackRequest
   {
      std::string filename;
      TransitionType transition;
      std::chrono::milliseconds duration{2000};  // for crossfade or fadeout
   };

   class MusicPlayer
   {
   public:
      void update(const sf::Time& dt);
      void queueTrack(const TrackRequest& request);
      void stop();

   private:
      void beginTransition(const TrackRequest& request);
      float volume() const;

      sf::Music _music_a;
      sf::Music _music_b;
      sf::Music* _current = nullptr;
      sf::Music* _next = nullptr;
      bool _using_a = true;

      std::optional<TrackRequest> _pending_request;
      bool _is_crossfading = false;
      std::chrono::milliseconds _crossfade_duration{};
      std::chrono::milliseconds _crossfade_elapsed{};
   };

   static Audio& getInstance();

   void adjustActiveSampleVolume();

   void addSample(const std::string& sample);
   std::optional<int32_t> playSample(const PlayInfo& play_info);
   void stopSample(const std::string& name);
   void stopSample(int32_t thread);
   void setVolume(int32_t thread, float volume);
   void setPosition(int32_t thread, const sf::Vector2f pos);

   MusicPlayer& getMusicPlayer();

private:
   void initializeSamples();
   std::shared_ptr<sf::SoundBuffer> loadFile(const std::string& filename);
   void debug();

   std::mutex _mutex;
   std::unordered_map<std::string, std::shared_ptr<sf::SoundBuffer>> _sound_buffers;
   std::array<SoundThread, 50> _sound_threads;
   MusicPlayer _music_player;
};
