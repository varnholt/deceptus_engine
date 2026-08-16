#pragma once

#ifdef DECEPTUS_VRSFML

#include <SFML/Audio.hpp>

#include <memory>

///
/// \brief Provides the single playback device shared by the sound and the music backend.
///
/// Both backends used to open a device of their own. On the Switch that is actively broken: libnx
/// exposes one global audio sink through audout, so two devices mean two miniaudio audio threads
/// interleaving their buffers into the same queue. Both streams then come out chopped - far worse on
/// music, which is continuous, than on effects, which are short and intermittent.
///
/// One device lets miniaudio mix the two itself, which is what a playback device is for.
///
namespace PlaybackDeviceProvider
{

///
/// \brief Returns the shared playback device, or nullptr when no audio device is available.
///
/// Callers keep the returned pointer for as long as they use the device; the device is destroyed
/// once the last of them lets go. Only the main thread creates the backends, so no locking is
/// needed here.
///
inline std::shared_ptr<sf::PlaybackDevice> get()
{
   // held weakly rather than as a static shared_ptr on purpose. A static owner would keep the device
   // alive until static destruction, which runs after the audio context has already shut audout
   // down, and tearing a playback device down after its backend is gone is not something worth
   // relying on. Weakly held, the device instead dies with the backends that use it, during ordinary
   // shutdown.
   static std::weak_ptr<sf::PlaybackDevice> weak_device;

   if (auto device = weak_device.lock())
   {
      return device;
   }

   auto handle = sf::AudioContext::getDefaultPlaybackDeviceHandle();
   if (!handle.hasValue())
   {
      return nullptr;
   }

   auto device = std::make_shared<sf::PlaybackDevice>(*handle);
   weak_device = device;

   return device;
}

}  // namespace PlaybackDeviceProvider

#endif  // DECEPTUS_VRSFML
