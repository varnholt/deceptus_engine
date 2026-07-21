#include "audiobackend.h"

#ifndef __EMSCRIPTEN__

#include "framework/tools/log.h"

#include <SFML/Audio.hpp>

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
/// \brief desktop backend backed by vanilla SFML 3 sound buffers and sf::Sound instances.
///
/// Buffers are cached as shared_ptr so the sounds referencing them stay valid, and a
/// sound slot is reused via setBuffer to avoid reallocating on every playback.
class AudioBackendDesktop : public AudioBackend
{
public:
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

      auto buffer = std::make_shared<sf::SoundBuffer>();

      auto start_time = std::chrono::high_resolution_clock::now();
      const bool success = buffer->loadFromFile(full_path);
      auto end_time = std::chrono::high_resolution_clock::now();

      auto load_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

      if (!success)
      {
         Log::Error() << "unable to load file: " << sample_name;
         return false;
      }
      else if (load_duration.count() >= 100)
      {
         Log::Info() << "Audio load time for " << sample_name << ": " << load_duration.count() << " ms (Main thread - may cause hiccups)";
      }

      if (buffer->getChannelCount() < 2)
      {
         Log::Warning() << sample_name << " seems to be mono :(";
      }

      _sound_buffers[sample_name] = buffer;
      return true;
   }

   std::unique_ptr<sf::Sound> prepareSound(std::unique_ptr<sf::Sound> existing, const std::string& sample_name) override
   {
      const auto it = _sound_buffers.find(sample_name);
      if (it == _sound_buffers.end())
      {
         return nullptr;
      }

      if (existing == nullptr)
      {
         existing = std::make_unique<sf::Sound>(*it->second);
      }
      else
      {
         existing->setBuffer(*it->second);
      }

      return existing;
   }

   bool isActive(const sf::Sound& sound) const override
   {
      return sound.getStatus() != sf::Sound::Status::Stopped;
   }

   float volumeScale() const override
   {
      return 100.0f;
   }

   void setListenerPosition(const sf::Vector2f& position) override
   {
      // The default listener's up vector is (0, 1, 0)
      // sf::Listener::setUpVector
      sf::Listener::setPosition({position.x, position.y, 0.0f});
   }

private:
   std::unordered_map<std::string, std::shared_ptr<sf::SoundBuffer>> _sound_buffers;  //!< cached sound buffers keyed by filename
};
}  // namespace

std::unique_ptr<AudioBackend> AudioBackend::create()
{
   return std::make_unique<AudioBackendDesktop>();
}

#endif
