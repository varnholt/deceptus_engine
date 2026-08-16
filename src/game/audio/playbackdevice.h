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
inline sf::PlaybackDevice* get()
{
   static std::unique_ptr<sf::PlaybackDevice> device = []() -> std::unique_ptr<sf::PlaybackDevice>
   {
      auto handle = sf::AudioContext::getDefaultPlaybackDeviceHandle();
      if (!handle.hasValue())
      {
         return nullptr;
      }

      return std::make_unique<sf::PlaybackDevice>(*handle);
   }();

   return device.get();
}

}  // namespace PlaybackDeviceProvider

#endif  // DECEPTUS_VRSFML
