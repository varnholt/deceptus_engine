#include "audiobackend.h"

#ifdef __EMSCRIPTEN__

#include "framework/tools/log.h"

#include <SFML/Audio.hpp>
#include <SFML/System.hpp>

#include <chrono>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

namespace
{
const std::string sfx_path = "data/sounds/";
}  // namespace

namespace
{
/// \brief Emscripten backend backed by VRSFML sound buffers and device-bound sf::Sound instances.
///
/// A single playback device is created up front from the default device handle. Buffers are
/// cached by value in the map (a stable address the sounds can reference), and every playback
/// constructs a fresh sound bound to that device because VRSFML sounds must carry the device.
class AudioBackendEmscripten : public AudioBackend
{
public:
   AudioBackendEmscripten()
   {
      auto handle = sf::AudioContext::getDefaultPlaybackDeviceHandle();
      if (handle.hasValue())
      {
         _playback_device = std::make_unique<sf::PlaybackDevice>(*handle);
      }
   }

   bool hasSample(const std::string& sample_name) const override
   {
      return _sound_buffers.find(sample_name) != _sound_buffers.end();
   }

   bool loadSample(const std::string& sample_name) override
   {
      const std::string full_path = sfx_path + sample_name;
      if (!std::filesystem::exists(full_path))
      {
         Log::Error() << "audio file does not exist: " << sample_name;
         return false;
      }

      auto start_time = std::chrono::high_resolution_clock::now();
      auto buffer = sf::SoundBuffer::loadFromFile(sf::Path{full_path});
      auto end_time = std::chrono::high_resolution_clock::now();

      auto load_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

      if (!buffer.hasValue())
      {
         Log::Error() << "unable to load file: " << sample_name;
         return false;
      }
      else if (load_duration.count() >= 100)
      {
         Log::Info() << "Audio load time for " << sample_name << ": " << load_duration.count() << " ms (Main thread - may cause hiccups)";
      }

      if ((*buffer).getChannelCount() < 2)
      {
         Log::Warning() << sample_name << " seems to be mono :(";
      }

      _sound_buffers.emplace(sample_name, std::move(*buffer));
      return true;
   }

   std::unique_ptr<sf::Sound> prepareSound(std::unique_ptr<sf::Sound> /*existing*/, const std::string& sample_name) override
   {
      if (!_playback_device)
      {
         return nullptr;
      }

      const auto it = _sound_buffers.find(sample_name);
      if (it == _sound_buffers.end())
      {
         return nullptr;
      }

      return std::make_unique<sf::Sound>(*_playback_device, it->second);
   }

   bool isActive(const sf::Sound& sound) const override
   {
      return sound.isPlaying();
   }

   float volumeScale() const override
   {
      return 1.0f;
   }

   void setListenerPosition(const sf::Vector2f& position) override
   {
      if (!_playback_device)
      {
         return;
      }

      sf::Listener listener;
      listener.position = {position.x, position.y, 0.0f};
      _playback_device->applyListener(listener);
   }

private:
   std::unique_ptr<sf::PlaybackDevice> _playback_device;             //!< owned playback device; null if audio system is unavailable
   std::unordered_map<std::string, sf::SoundBuffer> _sound_buffers;  //!< cached sound buffers keyed by filename
};
}  // namespace

std::unique_ptr<AudioBackend> AudioBackend::create()
{
   return std::make_unique<AudioBackendEmscripten>();
}

#endif
