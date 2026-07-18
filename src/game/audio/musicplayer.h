#ifndef MUSICPLAYER_H
#define MUSICPLAYER_H

#include <SFML/Audio.hpp>
#ifdef __EMSCRIPTEN__
#include <SFML/System.hpp>
#endif
#include <array>
#include <chrono>
#include <cstddef>
#include <mutex>
#include <optional>
#include <vector>

#include "game/audio/musicplayertypes.h"

/// \brief singleton music playback controller supporting queued transitions, crossfades, and playlists.
class MusicPlayer
{
public:
   /// \brief request payload describing which track to play and how to transition to it.
   struct TrackRequest
   {
      std::string filename;
      MusicPlayerTypes::TransitionType transition;
      std::chrono::milliseconds duration{2000};  // for crossfade or fadeout
      MusicPlayerTypes::PostPlaybackAction post_action = MusicPlayerTypes::PostPlaybackAction::None;
   };

   /// \brief returns the global music-player singleton instance.
   /// \return reference to the shared music player.
   static MusicPlayer& getInstance();

   /// \brief advances transition state, processes queued requests, and handles end-of-track actions.
   /// \param dt elapsed frame time since the previous update.
   void update(const sf::Time& dt);

   /// \brief enqueues a track request to be processed by the next update tick.
   /// \param request track file, transition strategy, duration, and post-playback behavior.
   void queueTrack(const TrackRequest& request);

   /// \brief stops both internal music streams and clears pending transition state.
   void stop();

   /// \brief replaces the playlist used by play-next post actions and resets playlist index.
   /// \param playlist ordered list of track filenames.
   void setPlaylist(const std::vector<std::string>& playlist);

   /// \brief reapplies configured music volume to the currently active stream.
   void adjustActiveMusicVolume();

private:
   /// \brief constructs the player and opens placeholder tracks for both stream slots.
   MusicPlayer();

   /// \brief loads the requested track into the inactive slot and starts the selected transition behavior.
   /// \param request track request to execute immediately.
   void beginTransition(const TrackRequest& request);

   /// \brief computes effective music volume from master and music configuration values.
   /// \return normalized sf::Music volume value in the 0-100 range.
   float volume() const;

   /// \brief updates crossfade interpolation and swaps active slots when the fade completes.
   /// \param dt elapsed time step for fade progression.
   void updateCrossfade(std::chrono::milliseconds dt);

   /// \brief updates fade-out progression and optionally starts a queued replacement track when complete.
   /// \param dt elapsed time step for fade progression.
   void updateFadeOut(std::chrono::milliseconds dt);

   /// \brief executes the pending track request according to its transition mode when conditions are met.
   void processPendingRequest();

   /// \brief applies post-playback behavior when the current stream is no longer playing.
   void handleTrackFinished();

   mutable std::mutex _mutex;

#ifdef __EMSCRIPTEN__
   /// \brief returns a pointer to the currently active music stream slot, or nullptr if not loaded.
   sf::Music* currentMusic();

   /// \brief returns a pointer to the inactive music stream slot used for upcoming transitions, or nullptr if not loaded.
   sf::Music* nextMusic();

   std::unique_ptr<sf::PlaybackDevice> _playback_device;  //!< owned playback device; null if audio context is unavailable
   std::array<std::vector<std::byte>, 2>
      _music_data;  //!< compressed track bytes backing each reader; must outlive the MusicReader (openFromMemory references, not copies)
   std::array<std::unique_ptr<sf::MusicReader>, 2> _music_readers;  //!< music reader (memory source) for each stream slot
   std::array<std::unique_ptr<sf::Music>, 2> _music;                //!< music stream for each slot; null until first track is loaded
#else
   /// \brief returns the currently active music stream slot.
   /// \return reference to the active sf::Music instance.
   sf::Music& current();

   /// \brief returns the inactive music stream slot used for upcoming transitions.
   /// \return reference to the inactive sf::Music instance.
   sf::Music& next();

   std::array<sf::Music, 2> _music;
#endif
   int32_t _current_index = 0;  // 0 or 1

   MusicPlayerTypes::MusicTransitionState _transition_state = MusicPlayerTypes::MusicTransitionState::None;

   std::chrono::milliseconds _crossfade_elapsed{};
   std::chrono::milliseconds _crossfade_duration{};

   std::chrono::milliseconds _fade_out_elapsed{};
   std::chrono::milliseconds _fade_out_duration{};

   MusicPlayerTypes::PostPlaybackAction _post_action = MusicPlayerTypes::PostPlaybackAction::None;
   std::optional<TrackRequest> _pending_request;

   std::vector<std::string> _playlist;
   std::size_t _playlist_index = 0;
};

#endif  // MUSICPLAYER_H
