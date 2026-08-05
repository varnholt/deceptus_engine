#ifndef AUDIOBACKEND_H
#define AUDIOBACKEND_H

#include <SFML/Audio.hpp>
#ifdef __EMSCRIPTEN__
#include <SFML/System.hpp>
#endif

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

/// \brief platform-specific sound-effect plumbing for the Audio mixer.
///
/// Audio owns the platform-agnostic sample thread pool and the find-a-free-slot
/// orchestration and is fully platform-agnostic. AudioBackend abstracts the handful of
/// things that genuinely differ between the desktop (vanilla SFML 3) and Emscripten
/// (VRSFML) builds: the playback-device model, how a decoded buffer is cached (a value in
/// the map vs a shared_ptr), how a sound is bound to that buffer, how "still occupying its
/// slot" is queried, the listener update, and the per-platform volume scale. Exactly one
/// concrete backend is compiled per platform and created via create().
class AudioBackend
{
public:
   virtual ~AudioBackend() = default;

   /// \brief read-only view of the interleaved pcm data of one cached sample buffer.
   struct SampleData
   {
      const std::int16_t* _samples{nullptr};  //!< interleaved 16 bit pcm data owned by the cached buffer
      std::uint64_t _sample_count{0};         //!< number of samples across all channels
      std::uint32_t _channel_count{0};        //!< number of interleaved channels
      std::uint32_t _sample_rate{0};          //!< samples per second and channel
   };

   /// \brief returns a view of the pcm data of a cached sample.
   /// \param sample_name key of the cached buffer.
   /// \return sample data view, or std::nullopt when the buffer is not cached.
   virtual std::optional<SampleData> getSampleData(const std::string& sample_name) const = 0;

   /// \brief returns whether a buffer for the given sample is already cached.
   /// \param sample_name key of the sample buffer.
   /// \return true if the buffer has been loaded.
   virtual bool hasSample(const std::string& sample_name) const = 0;

   /// \brief loads and caches a sample buffer keyed by its filename.
   /// \param sample_name sample filename relative to the sfx directory.
   /// \return true if the buffer was loaded and cached.
   virtual bool loadSample(const std::string& sample_name) = 0;

   /// \brief (re)creates a sound bound to the cached buffer for the given sample.
   ///
   /// The desktop backend reuses the passed-in instance to avoid reallocating; the
   /// Emscripten backend always constructs a fresh, device-bound instance. Returns null
   /// when the sample buffer or the playback device is unavailable.
   /// \param existing the slot's current sound, or null.
   /// \param sample_name key of the cached buffer to bind.
   /// \return owning pointer to the prepared sound, or null on failure.
   virtual std::unique_ptr<sf::Sound> prepareSound(std::unique_ptr<sf::Sound> existing, const std::string& sample_name) = 0;

   /// \brief returns whether the given sound still occupies its slot.
   /// \param sound sound instance to query.
   /// \return true while the sound is not stopped.
   virtual bool isActive(const sf::Sound& sound) const = 0;

   /// \brief returns the per-platform gain scale applied on top of the configured volumes.
   /// \return volume scale factor.
   virtual float volumeScale() const = 0;

   /// \brief updates the listener position on the playback device.
   /// \param position world position in pixels (z is set to 0).
   virtual void setListenerPosition(const sf::Vector2f& position) = 0;

   /// \brief creates the backend implementation for the current platform.
   /// \return owning pointer to the platform backend.
   static std::unique_ptr<AudioBackend> create();
};

#endif  // AUDIOBACKEND_H
