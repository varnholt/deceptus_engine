#pragma once

#include "game/audio/audiobackend.h"

#include <SFML/Audio.hpp>
#ifdef DECEPTUS_VRSFML
#include <SFML/System.hpp>
#endif
#include <array>
#include <atomic>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

/// \brief singleton sound-effects manager that caches buffers and plays them on a fixed thread pool of sf::Sound instances.
class Audio
{
public:
   /// \brief constructs the audio manager and preloads commonly used sample files.
   Audio();

   /// \brief stops all active sounds and marks the instance as shut down.
   ~Audio();

   /// \brief metadata describing one audio file name.
   struct Track
   {
      std::string _filename;
   };

   /// \brief playback request options used when starting a sound sample.
   struct PlayInfo
   {
      /// \brief constructs an empty playback request.
      PlayInfo() = default;

      /// \brief constructs a request for a named sample with default volume and looping.
      /// \param sample_name key of the sample buffer to play.
      PlayInfo(const std::string& sample_name) : _sample_name(sample_name)
      {
      }

      /// \brief constructs a request with explicit sample name and relative volume multiplier.
      /// \param sample_name key of the sample buffer to play.
      /// \param volume per-sample volume multiplier before global audio settings are applied.
      PlayInfo(const std::string& sample_name, float volume) : _sample_name(sample_name), _volume(volume)
      {
      }

      /// \brief constructs a request with explicit sample name, volume multiplier, and loop flag.
      /// \param sample_name key of the sample buffer to play.
      /// \param volume per-sample volume multiplier before global audio settings are applied.
      /// \param looped true to play the sample in a loop until stopped.
      PlayInfo(const std::string& sample_name, float volume, bool looped) : _sample_name(sample_name), _volume(volume), _looped(looped)
      {
      }

      std::string _sample_name;
      float _volume = 1.0f;
      bool _looped = false;
      std::optional<sf::Vector3f> _pos;
   };

   /// \brief reusable playback slot containing one sf::Sound instance and its active request metadata.
   struct SoundThread
   {
      std::string _filename;
      std::unique_ptr<sf::Sound> _sound;
      PlayInfo _play_info;
      uint32_t _generation{0};  //!< bumped every time this slot is handed to a new sample

      /// \brief applies effective volume to the slot using master and sfx configuration multipliers.
      /// \param volume per-sample volume multiplier in normalized units.
      void setVolume(float volume);

      /// \brief sets 3d sound position from a 2d world position.
      /// \param pos world position in pixels.
      void setPosition(const sf::Vector2f& pos);
   };

   /// \brief returns the global audio singleton instance.
   /// \return reference to the shared audio manager.
   static Audio& getInstance();

   /// \brief reapplies configured volume scaling to all currently playing sample threads.
   void adjustActiveSampleVolume();

   /// \brief updates the listener position on the playback device.
   /// \param pos world position in pixels (z is set to 0).
   void updateListenerPosition(const sf::Vector2f& pos);

   /// \brief loads and caches a sample buffer if it has not been loaded yet.
   /// \param sample sample filename relative to the sfx directory.
   void addSample(const std::string& sample);

   /// \brief returns the playback duration of a cached sample.
   /// \param sample_name sample filename to look up.
   /// \return duration of the sample, or std::nullopt when the sample is not cached.
   std::optional<sf::Time> getSampleDuration(const std::string& sample_name);

   /// \brief returns a counter that changes whenever a sound thread is handed to a new sample.
   ///
   /// A thread index alone does not identify a playback: once a sample stops, its slot is recycled and
   /// handed to the next caller, so a stale index would let one owner read, re-volume or even stop
   /// another owner's sound. Callers that hold on to a thread index across frames should capture this
   /// counter right after playSample and compare before acting on the thread again.
   /// \param thread index of the sound thread to query.
   /// \return current generation of that thread.
   uint32_t getPlaybackGeneration(int32_t thread);

   /// \brief returns how loud the sample on one sound thread is at its current playback position.
   ///
   /// The value comes from a per-sample loudness envelope built from the decoded pcm data on first
   /// use and normalized to the loudest passage of that sample, so 1.0 is the sample's own peak
   /// rather than an absolute level. Callers can use this to drive gameplay or visuals from what is
   /// actually audible right now.
   /// \param thread index of the sound thread to sample.
   /// \return normalized loudness in 0..1, or std::nullopt when the thread plays nothing.
   std::optional<float> getSampleLoudness(int32_t thread);

   /// \brief starts sample playback on the first free sound thread.
   /// \param play_info playback request containing sample name, gain, looping, and optional position.
   /// \return thread index used for playback, or std::nullopt when no slot or sample is available.
   std::optional<int32_t> playSample(const PlayInfo& play_info);

   /// \brief stops all currently playing threads whose filename matches the given sample name.
   /// \param name sample filename to stop.
   void stopSample(const std::string& name);

   /// \brief stops playback for one sound thread by index.
   /// \param thread index of the sound thread to stop.
   void stopSample(int32_t thread);

   /// \brief updates volume for one sound thread.
   /// \param thread index of the sound thread to modify.
   /// \param volume per-sample volume multiplier in normalized units.
   void setVolume(int32_t thread, float volume);

   /// \brief updates 2d position for one sound thread.
   /// \param thread index of the sound thread to modify.
   /// \param pos world position in pixels.
   void setPosition(int32_t thread, const sf::Vector2f pos);

private:
   /// \brief preloads a fixed set of frequently used game sound effects.
   void initializeSamples();

   /// \brief prints how many sound threads are currently free for playback.
   void debug();

   /// \brief returns the loudness envelope of a cached sample, building it on first use.
   ///
   /// Must be called with _mutex held.
   /// \param sample_name sample filename to look up.
   /// \return normalized rms buckets, or nullptr when the sample is not cached or carries no data.
   const std::vector<float>* getLoudnessEnvelope(const std::string& sample_name);

   std::mutex _mutex;
   std::atomic<bool> _stopped = false;
   std::unique_ptr<AudioBackend> _backend;  //!< platform-specific device, buffer cache, and sound plumbing
   std::array<SoundThread, 50> _sound_threads;
   std::map<std::string, std::vector<float>> _loudness_envelopes;  //!< lazily built rms buckets keyed by sample filename
};
