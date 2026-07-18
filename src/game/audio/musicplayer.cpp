#include "musicplayer.h"

#include "framework/tools/log.h"
#include "game/config/gameconfiguration.h"

#include <chrono>
#include <filesystem>
#ifdef __EMSCRIPTEN__
#include <cstddef>
#include <fstream>
#include <string>
#include <vector>
#endif

#ifdef __EMSCRIPTEN__
namespace
{
// On Emscripten, miniaudio runs the audio callback on the AudioWorklet thread (a
// WASM Worker). Streamed music decodes inside that callback, and filesystem
// syscalls proxied from the worklet thread cannot be awaited, so file-backed
// sf::Music produces silence. Reading the whole compressed track into memory up
// front (on the main thread) lets the worklet decode it without touching the
// filesystem. The returned buffer must outlive the MusicReader because
// openFromMemory references it rather than copying.
std::vector<std::byte> readFileBytes(const std::string& path)
{
   std::ifstream file(path, std::ios::binary | std::ios::ate);
   if (!file)
   {
      return {};
   }

   const auto size = static_cast<std::size_t>(file.tellg());
   file.seekg(0, std::ios::beg);

   std::vector<std::byte> bytes(size);
   if (size > 0u && !file.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(size)))
   {
      return {};
   }

   return bytes;
}
}  // namespace
#endif

MusicPlayer& MusicPlayer::getInstance()
{
   static MusicPlayer instance;
   return instance;
}

MusicPlayer::MusicPlayer()
{
#ifdef __EMSCRIPTEN__
   auto handle = sf::AudioContext::getDefaultPlaybackDeviceHandle();
   if (!handle.hasValue())
   {
      return;
   }
   _playback_device = std::make_unique<sf::PlaybackDevice>(*handle);

   for (auto music_slot_index = 0u; music_slot_index < _music.size(); ++music_slot_index)
   {
      _music_data[music_slot_index] = readFileBytes("data/music/empty.ogg");
      if (_music_data[music_slot_index].empty())
      {
         continue;
      }

      auto reader = sf::MusicReader::openFromMemory(_music_data[music_slot_index].data(), _music_data[music_slot_index].size());
      if (reader.hasValue())
      {
         _music_readers[music_slot_index] = std::make_unique<sf::MusicReader>(std::move(*reader));
         _music[music_slot_index] = std::make_unique<sf::Music>(*_playback_device, *_music_readers[music_slot_index]);
         _music[music_slot_index]->setRelativeToListener(true);
      }
   }
#else
   if (!_music[0].openFromFile("data/music/empty.ogg"))
   {
   }

   if (!_music[1].openFromFile("data/music/empty.ogg"))
   {
   }

   _music[0].setRelativeToListener(true);
   _music[1].setRelativeToListener(true);
#endif
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

#ifdef __EMSCRIPTEN__
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
#else
   next().setVolume(volume() * normalized_time);
   current().setVolume(volume() * (1.0f - normalized_time));

   if (_crossfade_elapsed >= _crossfade_duration)
   {
      current().stop();
      current().setVolume(volume());
      _current_index = 1 - _current_index;  // swap
      _transition_state = MusicPlayerTypes::MusicTransitionState::None;
      _pending_request.reset();
   }
#endif
}

void MusicPlayer::updateFadeOut(std::chrono::milliseconds dt)
{
   _fade_out_elapsed += dt;
   const auto normalized_time = std::min(1.0f, static_cast<float>(_fade_out_elapsed.count()) / _fade_out_duration.count());

#ifdef __EMSCRIPTEN__
   if (auto* cur_music = currentMusic())
   {
      cur_music->setVolume(volume() * (1.0f - normalized_time));
   }
#else
   current().setVolume(volume() * (1.0f - normalized_time));
#endif

   if (_fade_out_elapsed >= _fade_out_duration)
   {
#ifdef __EMSCRIPTEN__
      if (auto* cur_music = currentMusic())
      {
         cur_music->stop();
      }
#else
      current().stop();
#endif

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
#ifdef __EMSCRIPTEN__
         if (currentMusic() == nullptr || !currentMusic()->isPlaying())
#else
         if (current().getStatus() != sf::SoundStream::Status::Playing)
#endif
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
#ifdef __EMSCRIPTEN__
   if (currentMusic() != nullptr && currentMusic()->isPlaying())
   {
      return;
   }
#else
   if (current().getStatus() == sf::SoundStream::Status::Playing)
   {
      return;
   }
#endif

   switch (_post_action)
   {
      case MusicPlayerTypes::PostPlaybackAction::None:
      {
         break;
      }

      case MusicPlayerTypes::PostPlaybackAction::Loop:
      {
#ifdef __EMSCRIPTEN__
         if (auto* cur_music = currentMusic())
         {
            cur_music->play();
         }
#else
         current().play();
#endif
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

#ifdef __EMSCRIPTEN__
sf::Music* MusicPlayer::currentMusic()
{
   return _music[_current_index].get();
}

sf::Music* MusicPlayer::nextMusic()
{
   return _music[1 - _current_index].get();
}
#else
sf::Music& MusicPlayer::current()
{
   return _music[_current_index];
}

sf::Music& MusicPlayer::next()
{
   return _music[1 - _current_index];
}
#endif

void MusicPlayer::queueTrack(const TrackRequest& request)
{
   std::scoped_lock lock(_mutex);
   _pending_request = request;
}

void MusicPlayer::stop()
{
   std::scoped_lock lock(_mutex);

#ifdef __EMSCRIPTEN__
   for (auto& music_ptr : _music)
   {
      if (music_ptr)
      {
         music_ptr->stop();
      }
   }
#else
   for (auto& music : _music)
   {
      music.stop();
   }
#endif

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
#ifdef __EMSCRIPTEN__
   if (auto* cur_music = currentMusic())
   {
      cur_music->setVolume(volume());
   }
#else
   current().setVolume(volume());
#endif
}

void MusicPlayer::beginTransition(const TrackRequest& request)
{
#ifdef __EMSCRIPTEN__
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

   auto track_bytes = readFileBytes(request.filename);
   if (track_bytes.empty())
   {
      Log::Error() << "unable to load music file: " << request.filename;
      return;
   }

   auto new_reader = sf::MusicReader::openFromMemory(track_bytes.data(), track_bytes.size());
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

   // Tear down the slot's existing stream and reader before replacing its backing
   // bytes: the old reader references _music_data[next_index] via openFromMemory.
   if (_music[next_index])
   {
      _music[next_index]->stop();
   }
   _music[next_index].reset();
   _music_readers[next_index].reset();

   // Moving the vector transfers the heap allocation without reallocating, so the
   // pointer captured by new_reader's MemoryInputStream stays valid.
   _music_data[next_index] = std::move(track_bytes);
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
#else
   auto& next_track = next();
   auto& current_track = current();

   // check if the file exists before attempting to load
   if (!std::filesystem::exists(request.filename))
   {
      Log::Error() << "music file does not exist: " << request.filename;
      return;
   }

   // time the music file loading operation
   auto start_time = std::chrono::high_resolution_clock::now();
   bool success = next_track.openFromFile(request.filename);
   auto end_time = std::chrono::high_resolution_clock::now();

   auto load_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

   if (!success)
   {
      Log::Error() << "unable to load music file: " << request.filename;
      return;
   }
   else if (load_duration.count() >= 100)
   {
      Log::Info() << "Music load time for " << request.filename << ": " << load_duration.count() << " ms (Main thread - may cause hiccups)";
   }

   if (request.transition == MusicPlayerTypes::TransitionType::Crossfade)
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

   if (request.transition == MusicPlayerTypes::TransitionType::ImmediateSwitch ||
       request.transition == MusicPlayerTypes::TransitionType::LetCurrentFinish)
   {
      _current_index = 1 - _current_index;
   }
#endif
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
