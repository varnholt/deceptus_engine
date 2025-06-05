#ifndef MUSICPLAYER_H
#define MUSICPLAYER_H

#include <SFML/Audio.hpp>
#include <array>
#include <mutex>
#include <optional>

#include "game/audio/musicplayertypes.h"

class MusicPlayer
{
public:
   struct TrackRequest
   {
      std::string filename;
      MusicPlayerTypes::TransitionType transition;
      std::chrono::milliseconds duration{2000};  // for crossfade or fadeout
      MusicPlayerTypes::PostPlaybackAction post_action = MusicPlayerTypes::PostPlaybackAction::None;
   };

   static MusicPlayer& getInstance();

   void update(const sf::Time& dt);
   void queueTrack(const TrackRequest& request);
   void stop();
   void setPlaylist(const std::vector<std::string>& playlist);
   void adjustActiveMusicVolume();

private:
   MusicPlayer();
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
