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

   enum class PostPlaybackAction
   {
      None,      // do nothing when the track ends
      Loop,      // restart the same track
      PlayNext,  // play next track in a list
   };

   enum class MusicTransitionState
   {
      None,
      Crossfading,
      FadingOut,
   };

   struct TrackRequest
   {
      std::string filename;
      TransitionType transition;
      std::chrono::milliseconds duration{2000};  // for crossfade or fadeout
      PostPlaybackAction post_action = PostPlaybackAction::None;
   };

   class MusicPlayer
   {
   public:
      void update(const sf::Time& dt);
      void queueTrack(const TrackRequest& request);
      void stop();
      void setPlaylist(const std::vector<std::string>& playlist);

   private:
      void beginTransition(const TrackRequest& request);
      float volume() const;
      void updateCrossfade(std::chrono::milliseconds dt);
      void updateFadeOut(std::chrono::milliseconds dt);
      void processPendingRequest();
      void handleTrackFinished();

      mutable std::mutex _mutex;

      sf::Music& current();
      sf::Music& next();

      std::array<sf::Music, 2> _music;
      int32_t _current_index = 0;  // 0 or 1

      MusicTransitionState _transition_state = MusicTransitionState::None;

      std::chrono::milliseconds _crossfade_elapsed{};
      std::chrono::milliseconds _crossfade_duration{};

      std::chrono::milliseconds _fade_out_elapsed{};
      std::chrono::milliseconds _fade_out_duration{};

      PostPlaybackAction _post_action = PostPlaybackAction::None;
      std::optional<TrackRequest> _pending_request;

      std::vector<std::string> _playlist;
      std::size_t _playlist_index = 0;
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
