#include "musicbackend.h"

#ifndef __EMSCRIPTEN__

#include <SFML/Audio.hpp>

#include <array>
#include <chrono>
#include <future>
#include <memory>
#include <string>

namespace
{
/// \brief desktop backend backed by vanilla SFML 3 file-streamed sf::Music instances.
///
/// The two streams live as plain value objects for the player's lifetime. Loading is
/// pushed onto a worker thread via std::async so opening/decoding a track never blocks
/// the game loop and stutters a frame.
class MusicBackendDesktop : public MusicBackend
{
public:
   MusicBackendDesktop()
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

   void play(int slot) override
   {
      _music[slot].play();
   }

   void stop(int slot) override
   {
      _music[slot].stop();
   }

   void setVolume(int slot, float volume) override
   {
      _music[slot].setVolume(volume);
   }

   bool isPlaying(int slot) const override
   {
      return _music[slot].getStatus() == sf::SoundStream::Status::Playing;
   }

   void beginLoad(int slot, const std::string& filename) override
   {
      _load_state[slot] = LoadState::Loading;

      // next() references a stable std::array slot, so capturing it by reference stays
      // valid for the lifetime of the load.
      auto& music = _music[slot];
      const std::string track_filename = filename;
      _load_future[slot] = std::async(std::launch::async, [&music, track_filename]() { return music.openFromFile(track_filename); });
   }

   bool isLoadReady(int slot) override
   {
      if (_load_state[slot] == LoadState::Ready)
      {
         return true;
      }

      if (_load_state[slot] == LoadState::Loading && _load_future[slot].valid() &&
          _load_future[slot].wait_for(std::chrono::seconds(0)) == std::future_status::ready)
      {
         _load_succeeded[slot] = _load_future[slot].get();
         _load_state[slot] = LoadState::Ready;
         return true;
      }

      return false;
   }

   bool loadSucceeded(int slot) const override
   {
      return _load_succeeded[slot];
   }

   void waitForLoad(int slot) override
   {
      if (_load_state[slot] == LoadState::Loading && _load_future[slot].valid())
      {
         _load_succeeded[slot] = _load_future[slot].get();
         _load_state[slot] = LoadState::Ready;
      }
   }

private:
   enum class LoadState
   {
      Idle,
      Loading,
      Ready
   };

   std::array<sf::Music, 2> _music;
   std::array<std::future<bool>, 2> _load_future;
   std::array<bool, 2> _load_succeeded{false, false};                       //!< result of the last completed load per slot
   std::array<LoadState, 2> _load_state{LoadState::Idle, LoadState::Idle};  //!< per-slot background load progress
};
}  // namespace

std::unique_ptr<MusicBackend> MusicBackend::create()
{
   return std::make_unique<MusicBackendDesktop>();
}

#endif
