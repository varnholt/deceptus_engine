#include "musicplayer.h"

#include "framework/tools/log.h"
#include "game/config/gameconfiguration.h"

#include <chrono>
#include <filesystem>

MusicPlayer& MusicPlayer::getInstance()
{
   static MusicPlayer instance;
   return instance;
}

MusicPlayer::MusicPlayer()
{
   auto handle = sf::AudioContext::getDefaultPlaybackDeviceHandle();
   if (!handle.hasValue())
   {
      return;
   }
   _playback_device = std::make_unique<sf::PlaybackDevice>(*handle);

   for (auto music_slot_index = 0u; music_slot_index < _music.size(); ++music_slot_index)
   {
      auto reader = sf::MusicReader::openFromFile(sf::Path{"data/music/empty.ogg"});
      if (reader.hasValue())
      {
         _music_readers[music_slot_index] = std::make_unique<sf::MusicReader>(std::move(*reader));
         _music[music_slot_index] = std::make_unique<sf::Music>(*_playback_device, *_music_readers[music_slot_index]);
         _music[music_slot_index]->setRelativeToListener(true);
      }
   }
}

void MusicPlayer::update(const sf::Time& dt)
{
   std::scoped_lock lock(_mutex);

   const auto dt_ms = std::chrono::milliseconds(dt.asMilliseconds());

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

   if (auto* next_music = nextMusic())
   {
      next_music->setVolume(volume() * normalized_time);
   }
   if (auto* cur_music = currentMusic())
   {
      cur_music->setVolume(volume() * (1.0f - normalized_time));
   }

   if (_crossfade_elapsed >= _crossfade_duration)
   {
      if (auto* cur_music = currentMusic())
      {
         cur_music->stop();
         cur_music->setVolume(volume());
      }
      _current_index = 1 - _current_index;  // swap
      _transition_state = MusicPlayerTypes::MusicTransitionState::None;
      _pending_request.reset();
   }
}

void MusicPlayer::updateFadeOut(std::chrono::milliseconds dt)
{
   _fade_out_elapsed += dt;
   const auto normalized_time = std::min(1.0f, static_cast<float>(_fade_out_elapsed.count()) / _fade_out_duration.count());

   if (auto* cur_music = currentMusic())
   {
      cur_music->setVolume(volume() * (1.0f - normalized_time));
   }

   if (_fade_out_elapsed >= _fade_out_duration)
   {
      if (auto* cur_music = currentMusic())
      {
         cur_music->stop();
      }

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

   const auto& request = _pending_request.value();

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
         if (currentMusic() == nullptr || !currentMusic()->isPlaying())
         {
            beginTransition(request);
            _pending_request.reset();
         }
         break;
      }

      case MusicPlayerTypes::TransitionType::Crossfade:
      {
         beginTransition(request);
         _transition_state = MusicPlayerTypes::MusicTransitionState::Crossfading;
         _crossfade_elapsed = std::chrono::milliseconds{0};
         _crossfade_duration = request.duration;
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
   if (currentMusic() != nullptr && currentMusic()->isPlaying())
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
         if (auto* cur_music = currentMusic())
         {
            cur_music->play();
         }
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

sf::Music* MusicPlayer::currentMusic()
{
   return _music[_current_index].get();
}

sf::Music* MusicPlayer::nextMusic()
{
   return _music[1 - _current_index].get();
}

void MusicPlayer::queueTrack(const TrackRequest& request)
{
   std::scoped_lock lock(_mutex);
   _pending_request = request;
}

void MusicPlayer::stop()
{
   std::scoped_lock lock(_mutex);

   for (auto& music_ptr : _music)
   {
      if (music_ptr)
      {
         music_ptr->stop();
      }
   }

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
   if (auto* cur_music = currentMusic())
   {
      cur_music->setVolume(volume());
   }
}

void MusicPlayer::beginTransition(const TrackRequest& request)
{
   if (!_playback_device)
   {
      return;
   }

   if (!std::filesystem::exists(request.filename))
   {
      Log::Error() << "music file does not exist: " << request.filename;
      return;
   }

   auto start_time = std::chrono::high_resolution_clock::now();
   auto new_reader = sf::MusicReader::openFromFile(sf::Path{request.filename});
   auto end_time = std::chrono::high_resolution_clock::now();

   auto load_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

   if (!new_reader.hasValue())
   {
      Log::Error() << "unable to load music file: " << request.filename;
      return;
   }
   else if (load_duration.count() >= 100)
   {
      Log::Info() << "Music load time for " << request.filename << ": " << load_duration.count() << " ms (Main thread - may cause hiccups)";
   }

   const auto next_index = 1 - _current_index;

   if (_music[next_index])
   {
      _music[next_index]->stop();
   }
   _music[next_index].reset();
   _music_readers[next_index] = std::make_unique<sf::MusicReader>(std::move(*new_reader));
   _music[next_index] = std::make_unique<sf::Music>(*_playback_device, *_music_readers[next_index]);
   _music[next_index]->setRelativeToListener(true);

   if (request.transition == MusicPlayerTypes::TransitionType::Crossfade)
   {
      _music[next_index]->setVolume(0.f);
   }
   else
   {
      _music[next_index]->setVolume(volume());
      if (_music[_current_index])
      {
         _music[_current_index]->stop();
      }
   }

   _music[next_index]->play();
   _post_action = request.post_action;

   if (request.transition == MusicPlayerTypes::TransitionType::ImmediateSwitch ||
       request.transition == MusicPlayerTypes::TransitionType::LetCurrentFinish)
   {
      _current_index = 1 - _current_index;
   }
}

float MusicPlayer::volume() const
{
   const auto& config = GameConfiguration::getInstance();
   const auto master = config._audio_volume_master * 0.01f;
   const auto music = config._audio_volume_music * 0.01f;
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
