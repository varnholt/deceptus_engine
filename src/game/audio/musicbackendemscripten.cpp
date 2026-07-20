#include "musicbackend.h"

#ifdef __EMSCRIPTEN__

#include <SFML/Audio.hpp>
#include <SFML/System.hpp>

#include <array>
#include <cstddef>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

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

/// \brief Emscripten backend backed by VRSFML sf::Music instances streamed from memory.
///
/// Each track is read fully into memory on the main thread and handed to the stream via
/// openFromMemory, because the AudioWorklet thread that decodes it cannot perform
/// filesystem syscalls. Loading is therefore synchronous; glitches are accepted on this
/// platform.
class MusicBackendEmscripten : public MusicBackend
{
public:
   MusicBackendEmscripten()
   {
      auto handle = sf::AudioContext::getDefaultPlaybackDeviceHandle();
      if (!handle.hasValue())
      {
         return;
      }

      _playback_device = std::make_unique<sf::PlaybackDevice>(*handle);

      loadIntoSlot(0, "data/music/empty.ogg");
      loadIntoSlot(1, "data/music/empty.ogg");
   }

   void play(int slot) override
   {
      if (_music[slot])
      {
         _music[slot]->play();
      }
   }

   void stop(int slot) override
   {
      if (_music[slot])
      {
         _music[slot]->stop();
      }
   }

   void setVolume(int slot, float volume) override
   {
      if (_music[slot])
      {
         _music[slot]->setVolume(volume);
      }
   }

   bool isPlaying(int slot) const override
   {
      return _music[slot] && _music[slot]->isPlaying();
   }

   void beginLoad(int slot, const std::string& filename) override
   {
      _load_succeeded[slot] = loadIntoSlot(slot, filename);
   }

   bool isLoadReady(int /*slot*/) override
   {
      return true;
   }

   bool loadSucceeded(int slot) const override
   {
      return _load_succeeded[slot];
   }

   void waitForLoad(int /*slot*/) override
   {
   }

private:
   /// \brief reads the track into memory and (re)creates the slot's stream from it.
   /// \param slot stream slot index (0 or 1) to load into.
   /// \param filename path of the track to open.
   /// \return true if a usable stream was created.
   bool loadIntoSlot(int slot, const std::string& filename)
   {
      if (!_playback_device)
      {
         return false;
      }

      auto track_bytes = readFileBytes(filename);
      if (track_bytes.empty())
      {
         return false;
      }

      auto new_reader = sf::MusicReader::openFromMemory(track_bytes.data(), track_bytes.size());
      if (!new_reader.hasValue())
      {
         return false;
      }

      // Tear down the slot's existing stream and reader before replacing its backing
      // bytes: the old reader references _music_data[slot] via openFromMemory.
      if (_music[slot])
      {
         _music[slot]->stop();
      }
      _music[slot].reset();
      _music_readers[slot].reset();

      // Moving the vector transfers the heap allocation without reallocating, so the
      // pointer captured by new_reader's MemoryInputStream stays valid.
      _music_data[slot] = std::move(track_bytes);
      _music_readers[slot] = std::make_unique<sf::MusicReader>(std::move(*new_reader));
      _music[slot] = std::make_unique<sf::Music>(*_playback_device, *_music_readers[slot]);
      _music[slot]->setRelativeToListener(true);

      return true;
   }

   std::unique_ptr<sf::PlaybackDevice> _playback_device;  //!< owned playback device; null if audio context is unavailable
   std::array<std::vector<std::byte>, 2>
      _music_data;  //!< compressed track bytes backing each reader; must outlive the MusicReader (openFromMemory references, not copies)
   std::array<std::unique_ptr<sf::MusicReader>, 2> _music_readers;  //!< music reader (memory source) for each stream slot
   std::array<std::unique_ptr<sf::Music>, 2> _music;                //!< music stream for each slot; null until first track is loaded
   std::array<bool, 2> _load_succeeded{false, false};               //!< result of the last completed load per slot
};
}  // namespace

std::unique_ptr<MusicBackend> MusicBackend::create()
{
   return std::make_unique<MusicBackendEmscripten>();
}

#endif
