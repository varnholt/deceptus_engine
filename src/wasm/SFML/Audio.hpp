#pragma once
#include <SFML/System.hpp>

// SFML Audio is disabled for WASM builds (SFML_BUILD_AUDIO OFF).
// Provide minimal compat stubs so audio.h / musicplayer.h compile cleanly.
// Runtime audio calls are no-ops; the game logic still compiles.

#ifndef __EMSCRIPTEN__
#include <SFML/Audio/Music.hpp>
#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/Audio/SoundStream.hpp>
#else
// ---- WASM stubs ----
#include <string>

namespace sf
{

struct SoundBuffer
{
   bool loadFromFile(const std::string&)
   {
      return false;
   }
   unsigned int getChannelCount() const
   {
      return 2;
   }
};

struct Sound
{
   enum class Status
   {
      Stopped,
      Paused,
      Playing
   };
   Sound() = default;
   Sound(const SoundBuffer&)
   {
   }
   void play()
   {
   }
   void stop()
   {
   }
   void pause()
   {
   }
   Status getStatus() const
   {
      return Status::Stopped;
   }
   void setBuffer(const SoundBuffer&)
   {
   }
   void setVolume(float)
   {
   }
   void setAttenuation(float)
   {
   }
   void setMinDistance(float)
   {
   }
   void setRelativeToListener(bool)
   {
   }
   void setPosition(float, float, float)
   {
   }
   void setLoop(bool)
   {
   }
   void setLooping(bool)
   {
   }
   bool getLoop() const
   {
      return false;
   }
   Vector3f position{};
};

struct SoundStream
{
   enum class Status
   {
      Stopped,
      Paused,
      Playing
   };
};

struct Music
{
   using Status = SoundStream::Status;
   bool openFromFile(const std::string&)
   {
      return false;
   }
   void play()
   {
   }
   void stop()
   {
   }
   void pause()
   {
   }
   SoundStream::Status getStatus() const
   {
      return SoundStream::Status::Stopped;
   }
   void setVolume(float)
   {
   }
   void setLoop(bool)
   {
   }
   bool getLoop() const
   {
      return false;
   }
   void setRelativeToListener(bool)
   {
   }
};

struct Listener
{
   static void setPosition(Vector3f)
   {
   }
   static void setDirection(Vector3f)
   {
   }
   static void setUpVector(Vector3f)
   {
   }
   static void setGlobalVolume(float)
   {
   }
};

}  // namespace sf
#endif
