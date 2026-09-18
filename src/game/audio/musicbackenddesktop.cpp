#include "musicbackend.h"

#ifndef DECEPTUS_VRSFML

#include "framework/tools/assetsource.h"

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
      loadFromAsset(0, "data/music/empty.ogg");
      loadFromAsset(1, "data/music/empty.ogg");

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

   void setLooping(int slot, bool looping) override
   {
      _music[slot].setLooping(looping);
   }

   void beginLoad(int slot, const std::string& filename) override
   {
      _load_state[slot] = LoadState::Loading;

      // next() references a stable std::array slot, so capturing it by reference stays
      // valid for the lifetime of the load.
      const std::string track_filename = filename;
      _load_future[slot] = std::async(std::launch::async, [this, slot, track_filename]() { return loadFromAsset(slot, track_filename); });
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

   /// \brief reads `filename` via AssetSource and (re)opens the slot's stream from the bytes.
   /// \return true if the stream was opened successfully.
   bool loadFromAsset(int slot, const std::string& filename)
   {
      auto file_contents = AssetSource::readFile(filename);
      if (!file_contents.has_value())
      {
         return false;
      }

      // openFromMemory() itself calls stop() first on whatever _music_data[slot] currently holds
      // (sf::Music::stop() seeks back to 0, which reads from the stream's current buffer) before
      // switching to the new one - so the old buffer has to stay valid and untouched until
      // openFromMemory() returns, not just until this function starts. The new bytes are opened
      // directly out of file_contents; a real ogg file is always far past the small-string-
      // optimization threshold, so moving it into _music_data[slot] afterwards keeps the exact
      // heap address openFromMemory just captured, which is what has to outlive the stream.
      const auto opened = _music[slot].openFromMemory(file_contents->data(), file_contents->size());
      _music_data[slot] = std::move(*file_contents);
      return opened;
   }

   std::array<sf::Music, 2> _music;
   std::array<std::string, 2>
      _music_data;  //!< compressed track bytes backing each stream; must outlive it (openFromMemory references, not copies)
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
