#ifndef MUSICBACKEND_H
#define MUSICBACKEND_H

#include <memory>
#include <string>

/// \brief platform-specific stream plumbing for the music player.
///
/// MusicPlayer owns the transition/crossfade/playlist state machine and is fully
/// platform-agnostic. MusicBackend abstracts the two things that genuinely differ
/// between the desktop (vanilla SFML 3) and Emscripten (VRSFML) builds: how a track
/// is opened and how a stream slot is addressed. Exactly one concrete backend is
/// compiled per platform and created via create().
///
/// Slot indices are 0 or 1 — the player double-buffers two streams so it can
/// crossfade or hand over between the active and the inactive slot.
class MusicBackend
{
public:
   virtual ~MusicBackend() = default;

   /// \brief starts playback of the stream in the given slot.
   /// \param slot stream slot index (0 or 1).
   virtual void play(int slot) = 0;

   /// \brief stops the stream in the given slot.
   /// \param slot stream slot index (0 or 1).
   virtual void stop(int slot) = 0;

   /// \brief sets the playback volume of the stream in the given slot.
   /// \param slot stream slot index (0 or 1).
   /// \param volume platform-scaled volume value to apply.
   virtual void setVolume(int slot, float volume) = 0;

   /// \brief returns whether the stream in the given slot is currently playing.
   /// \param slot stream slot index (0 or 1).
   /// \return true if the slot holds a stream that is playing.
   virtual bool isPlaying(int slot) const = 0;

   /// \brief begins loading a track into the given slot.
   ///
   /// The desktop backend opens the file on a background thread so the game loop never
   /// blocks; the Emscripten backend reads it into memory synchronously (an
   /// AudioWorklet-thread constraint). Either way the result is retrieved through
   /// isLoadReady() and loadSucceeded().
   /// \param slot stream slot index (0 or 1) to load into.
   /// \param filename path of the track to open.
   virtual void beginLoad(int slot, const std::string& filename) = 0;

   /// \brief returns whether the load started for the given slot has finished.
   /// \param slot stream slot index (0 or 1).
   /// \return true once the load result is available.
   virtual bool isLoadReady(int slot) = 0;

   /// \brief returns whether the most recently completed load for the slot succeeded.
   /// \param slot stream slot index (0 or 1).
   /// \return true if the last load opened a usable stream.
   virtual bool loadSucceeded(int slot) const = 0;

   /// \brief blocks until any in-flight load for the given slot has finished.
   /// \param slot stream slot index (0 or 1).
   virtual void waitForLoad(int slot) = 0;

   /// \brief creates the backend implementation for the current platform.
   /// \return owning pointer to the platform backend.
   static std::unique_ptr<MusicBackend> create();
};

#endif  // MUSICBACKEND_H
