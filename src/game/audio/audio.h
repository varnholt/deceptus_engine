#pragma once

#include <SFML/Audio.hpp>
#include <array>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>

/// \brief singleton sound-effects manager that caches buffers and plays them on a fixed thread pool of sf::Sound instances.
class Audio
{
public:
   /// \brief constructs the audio manager and preloads commonly used sample files.
   Audio();

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

   /// \brief loads and caches a sample buffer if it has not been loaded yet.
   /// \param sample sample filename relative to the sfx directory.
   void addSample(const std::string& sample);

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

   /// \brief loads an sf::SoundBuffer from the sfx folder and returns it when successful.
   /// \param filename sample filename relative to the sfx directory.
   /// \return loaded sound buffer, or nullptr when the file is missing or cannot be decoded.
   std::shared_ptr<sf::SoundBuffer> loadFile(const std::string& filename);

   /// \brief prints how many sound threads are currently free for playback.
   void debug();

   std::mutex _mutex;
   std::unordered_map<std::string, std::shared_ptr<sf::SoundBuffer>> _sound_buffers;
   std::array<SoundThread, 50> _sound_threads;
};
