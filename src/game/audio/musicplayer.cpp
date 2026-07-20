#include "musicplayer.h"

#include "framework/tools/log.h"
#include "game/config/gameconfiguration.h"

#include <algorithm>
#include <chrono>
#include <filesystem>

MusicPlayer& MusicPlayer::getInstance()
{
   static MusicPlayer instance;
   return instance;
}

MusicPlayer::MusicPlayer() : _backend(MusicBackend::create())
{
}

void MusicPlayer::update(const sf::Time& dt)
{
   std::scoped_lock lock(_mutex);

   const auto dt_ms = std::chrono::milliseconds(dt.asMilliseconds());

   // a track is being opened into the inactive slot; wait for it to finish before
   // starting playback or any further transition. The desktop backend loads on a worker
   // thread (so the frame never blocks), the Emscripten backend loads synchronously —
   // either way the state machine treats it the same.
   if (_loading_request.has_value())
   {
      const auto next_index = 1 - _current_index;
      if (_backend->isLoadReady(next_index))
      {
         activateLoadedTrack(_backend->loadSucceeded(next_index));
      }
      return;
   }

   switch (_transition_state)
   {
      case MusicPlayerTypes::MusicTransitionState::Crossfading:
      {
         updateCrossfade(dt_ms);
         return;
      }

      case MusicPlayerTypes::MusicTransitionState::FadingOut:
      {
         updateFadeOut(dt_ms);
         return;
      }

      case MusicPlayerTypes::MusicTransitionState::None:
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

   const auto next_index = 1 - _current_index;
   _backend->setVolume(next_index, volume() * normalized_time);
   _backend->setVolume(_current_index, volume() * (1.0f - normalized_time));

   if (_crossfade_elapsed >= _crossfade_duration)
   {
      _backend->stop(_current_index);
      _backend->setVolume(_current_index, volume());
      _current_index = next_index;  // swap
      _transition_state = MusicPlayerTypes::MusicTransitionState::None;
      _pending_request.reset();
   }
}

void MusicPlayer::updateFadeOut(std::chrono::milliseconds dt)
{
   _fade_out_elapsed += dt;
   const auto normalized_time = std::min(1.0f, static_cast<float>(_fade_out_elapsed.count()) / _fade_out_duration.count());

   _backend->setVolume(_current_index, volume() * (1.0f - normalized_time));

   if (_fade_out_elapsed >= _fade_out_duration)
   {
      _backend->stop(_current_index);

      if (_pending_request.has_value())
      {
         TrackRequest new_request = _pending_request.value();
         new_request.transition = MusicPlayerTypes::TransitionType::ImmediateSwitch;
         beginTransition(new_request);
      }

      _pending_request.reset();
      _transition_state = MusicPlayerTypes::MusicTransitionState::None;
   }
}

void MusicPlayer::processPendingRequest()
{
   if (!_pending_request.has_value())
   {
      return;
   }

   const auto request = _pending_request.value();

   switch (request.transition)
   {
      case MusicPlayerTypes::TransitionType::ImmediateSwitch:
      {
         beginTransition(request);
         _pending_request.reset();
         break;
      }

      case MusicPlayerTypes::TransitionType::LetCurrentFinish:
      {
         if (!_backend->isPlaying(_current_index))
         {
            beginTransition(request);
            _pending_request.reset();
         }
         break;
      }

      case MusicPlayerTypes::TransitionType::Crossfade:
      {
         // the crossfade only begins once the track finishes loading; activateLoadedTrack()
         // sets the Crossfading state. Clear the pending request so it is not re-processed
         // while the load is in flight.
         beginTransition(request);
         _pending_request.reset();
         break;
      }

      case MusicPlayerTypes::TransitionType::FadeOutThenNew:
      {
         if (_transition_state == MusicPlayerTypes::MusicTransitionState::None)
         {
            _transition_state = MusicPlayerTypes::MusicTransitionState::FadingOut;
            _fade_out_elapsed = std::chrono::milliseconds{0};
            _fade_out_duration = request.duration;
         }
         break;
      }
   }
}

void MusicPlayer::handleTrackFinished()
{
   if (_backend->isPlaying(_current_index))
   {
      return;
   }

   switch (_post_action)
   {
      case MusicPlayerTypes::PostPlaybackAction::None:
      {
         break;
      }

      case MusicPlayerTypes::PostPlaybackAction::Loop:
      {
         _backend->play(_current_index);
         break;
      }

      case MusicPlayerTypes::PostPlaybackAction::PlayNext:
      {
         if (!_playlist.empty())
         {
            _playlist_index = (_playlist_index + 1) % _playlist.size();
            queueTrack(
               {.filename = _playlist[_playlist_index],
                .transition = MusicPlayerTypes::TransitionType::ImmediateSwitch,
                .duration = std::chrono::milliseconds{0},
                .post_action = MusicPlayerTypes::PostPlaybackAction::PlayNext}
            );
         }
         break;
      }
   }
}

void MusicPlayer::queueTrack(const TrackRequest& request)
{
   std::scoped_lock lock(_mutex);
   _pending_request = request;
}

void MusicPlayer::stop()
{
   std::scoped_lock lock(_mutex);

   // wait for any in-flight background load to finish so the backend is not opening a
   // stream while we stop it.
   _backend->waitForLoad(1 - _current_index);
   _loading_request.reset();

   _backend->stop(0);
   _backend->stop(1);

   _pending_request.reset();
   _transition_state = MusicPlayerTypes::MusicTransitionState::None;
}

void MusicPlayer::setPlaylist(const std::vector<std::string>& playlist)
{
   std::scoped_lock lock(_mutex);

   _playlist = playlist;
   _playlist_index = 0;
}

void MusicPlayer::adjustActiveMusicVolume()
{
   _backend->setVolume(_current_index, volume());
}

void MusicPlayer::beginTransition(const TrackRequest& request)
{
   // check if the file exists before attempting to load
   if (!std::filesystem::exists(request.filename))
   {
      Log::Error() << "music file does not exist: " << request.filename;
      return;
   }

   // Load the track into the inactive slot. beginLoad is asynchronous on desktop and
   // synchronous on Emscripten; activateLoadedTrack() picks the result up through the
   // loading branch in update() once the backend reports it ready.
   _loading_request = request;
   _backend->beginLoad(1 - _current_index, request.filename);
}

void MusicPlayer::activateLoadedTrack(bool load_succeeded)
{
   const auto request = _loading_request.value();
   _loading_request.reset();

   if (!load_succeeded)
   {
      Log::Error() << "unable to load music file: " << request.filename;
      _transition_state = MusicPlayerTypes::MusicTransitionState::None;
      return;
   }

   const auto next_index = 1 - _current_index;

   if (request.transition == MusicPlayerTypes::TransitionType::Crossfade)
   {
      _backend->setVolume(next_index, 0.f);
   }
   else
   {
#ifdef __EMSCRIPTEN__
      _backend->setVolume(next_index, volume());
#else
      _backend->setVolume(next_index, 100.f);
#endif
      _backend->stop(_current_index);
   }

   _backend->play(next_index);
   _post_action = request.post_action;

   if (request.transition == MusicPlayerTypes::TransitionType::ImmediateSwitch ||
       request.transition == MusicPlayerTypes::TransitionType::LetCurrentFinish)
   {
      _current_index = next_index;
   }
   else if (request.transition == MusicPlayerTypes::TransitionType::Crossfade)
   {
      _transition_state = MusicPlayerTypes::MusicTransitionState::Crossfading;
      _crossfade_elapsed = std::chrono::milliseconds{0};
      _crossfade_duration = request.duration;
   }
}

float MusicPlayer::volume() const
{
   const auto& config = GameConfiguration::getInstance();
   const auto master = config._audio_volume_master * 0.01f;
#ifdef __EMSCRIPTEN__
   const auto music = config._audio_volume_music * 0.01f;
#else
   const auto music = config._audio_volume_music;
#endif
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
