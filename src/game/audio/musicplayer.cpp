#include "musicplayer.h"

#include "game/config/gameconfiguration.h"

MusicPlayer& MusicPlayer::getInstance()
{
   static MusicPlayer instance;
   return instance;
}

MusicPlayer::MusicPlayer()
{
   if (!_music[0].openFromFile("data/music/empty.ogg"))
   {
   }

   if (!_music[1].openFromFile("data/music/empty.ogg"))
   {
   }

   _music[0].setRelativeToListener(true);
   _music[1].setRelativeToListener(true);
}

void MusicPlayer::update(const sf::Time& dt)
{
   std::scoped_lock lock(_mutex);

   const auto dt_ms = std::chrono::milliseconds(dt.asMilliseconds());

   switch (_transition_state)
   {
      case MusicTransitionState::Crossfading:
      {
         updateCrossfade(dt_ms);
         return;
      }

      case MusicTransitionState::FadingOut:
      {
         updateFadeOut(dt_ms);
         return;
      }

      case MusicTransitionState::None:
      {
         break;
      }
   }

   processPendingRequest();
   handleTrackFinished();
}

void MusicPlayer::updateCrossfade(std::chrono::milliseconds dt)
{
   _crossfade_elapsed += dt;
   const auto normalized_time = std::min(1.0f, static_cast<float>(_crossfade_elapsed.count()) / _crossfade_duration.count());

   next().setVolume(volume() * normalized_time);
   current().setVolume(volume() * (1.0f - normalized_time));

   if (_crossfade_elapsed >= _crossfade_duration)
   {
      current().stop();
      current().setVolume(volume());
      _current_index = 1 - _current_index;  // swap
      _transition_state = MusicTransitionState::None;
      _pending_request.reset();
   }
}

void MusicPlayer::updateFadeOut(std::chrono::milliseconds dt)
{
   _fade_out_elapsed += dt;
   const auto normalized_time = std::min(1.0f, static_cast<float>(_fade_out_elapsed.count()) / _fade_out_duration.count());

   current().setVolume(volume() * (1.0f - normalized_time));

   if (_fade_out_elapsed >= _fade_out_duration)
   {
      current().stop();

      if (_pending_request.has_value())
      {
         TrackRequest new_request = _pending_request.value();
         new_request.transition = TransitionType::ImmediateSwitch;
         beginTransition(new_request);
      }

      _pending_request.reset();
      _transition_state = MusicTransitionState::None;
   }
}

void MusicPlayer::processPendingRequest()
{
   if (!_pending_request.has_value())
   {
      return;
   }

   const auto& request = _pending_request.value();

   switch (request.transition)
   {
      case TransitionType::ImmediateSwitch:
      {
         beginTransition(request);
         _pending_request.reset();
         break;
      }

      case TransitionType::LetCurrentFinish:
      {
         if (current().getStatus() != sf::SoundStream::Status::Playing)
         {
            beginTransition(request);
            _pending_request.reset();
         }
         break;
      }

      case TransitionType::Crossfade:
      {
         beginTransition(request);
         _transition_state = MusicTransitionState::Crossfading;
         _crossfade_elapsed = std::chrono::milliseconds{0};
         _crossfade_duration = request.duration;
         break;
      }

      case TransitionType::FadeOutThenNew:
      {
         if (_transition_state == MusicTransitionState::None)
         {
            _transition_state = MusicTransitionState::FadingOut;
            _fade_out_elapsed = std::chrono::milliseconds{0};
            _fade_out_duration = request.duration;
         }
         break;
      }
   }
}

void MusicPlayer::handleTrackFinished()
{
   if (current().getStatus() == sf::SoundStream::Status::Playing)
   {
      return;
   }

   switch (_post_action)
   {
      case PostPlaybackAction::None:
      {
         break;
      }

      case PostPlaybackAction::Loop:
      {
         current().play();
         break;
      }

      case PostPlaybackAction::PlayNext:
      {
         if (!_playlist.empty())
         {
            _playlist_index = (_playlist_index + 1) % _playlist.size();
            queueTrack(
               {.filename = _playlist[_playlist_index],
                .transition = TransitionType::ImmediateSwitch,
                .duration = std::chrono::milliseconds{0},
                .post_action = PostPlaybackAction::PlayNext}
            );
         }
         break;
      }
   }
}
sf::Music& MusicPlayer::current()
{
   return _music[_current_index];
}

sf::Music& MusicPlayer::next()
{
   return _music[1 - _current_index];
}

void MusicPlayer::queueTrack(const TrackRequest& request)
{
   std::scoped_lock lock(_mutex);
   _pending_request = request;
}

void MusicPlayer::stop()
{
   std::scoped_lock lock(_mutex);

   for (auto& music : _music)
   {
      music.stop();
   }

   _pending_request.reset();
   _transition_state = MusicTransitionState::None;
}

void MusicPlayer::setPlaylist(const std::vector<std::string>& playlist)
{
   std::scoped_lock lock(_mutex);

   _playlist = playlist;
   _playlist_index = 0;
}

void MusicPlayer::adjustActiveMusicVolume()
{
   current().setVolume(volume());
}

void MusicPlayer::beginTransition(const TrackRequest& request)
{
   auto& next_track = next();
   auto& current_track = current();

   if (!next_track.openFromFile(request.filename))
   {
      // optionally handle error
      return;
   }

   if (request.transition == TransitionType::Crossfade)
   {
      next_track.setVolume(0.f);
   }
   else
   {
      next_track.setVolume(100.f);
      current_track.stop();
   }

   next_track.play();
   _post_action = request.post_action;

   if (request.transition == TransitionType::ImmediateSwitch || request.transition == TransitionType::LetCurrentFinish)
   {
      _current_index = 1 - _current_index;
   }
}

float MusicPlayer::volume() const
{
   const auto& config = GameConfiguration::getInstance();
   const auto master = config._audio_volume_master * 0.01f;
   const auto music = config._audio_volume_music;
   return master * music;
}

/*


// simple usage
{
musicPlayer.queueTrack({
 .filename = "level1.ogg",
 .transition = TransitionType::Crossfade,
 .duration = std::chrono::milliseconds(3000),
 .post_action = PostPlaybackAction::Loop
});
}

// playlist mode
{
musicPlayer._playlist = {"level1.ogg", "level2.ogg", "boss_battle.ogg"};
musicPlayer._playlist_index = 0;

musicPlayer.queueTrack({
 .filename = musicPlayer._playlist[0],
 .transition = TransitionType::ImmediateSwitch,
 .duration = std::chrono::milliseconds(0),
 .post_action = PostPlaybackAction::PlayNext
});
}

// fade out then new
{
musicPlayer.queueTrack({
 .filename = "next_song.ogg",
 .transition = TransitionType::FadeOutThenNew,
 .duration = std::chrono::milliseconds(2000),
 .post_action = PostPlaybackAction::None
});
}

 */
