#ifndef SOUNDROTATION_H
#define SOUNDROTATION_H

#include <SFML/System.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace SoundList
{

/// \brief splits a semicolon separated sample list as it is written in tmx properties.
///
/// Reads `"sample_1.ogg;sample_2.ogg;sample_3.ogg"`; empty entries are dropped so a trailing
/// semicolon does not produce a sample that cannot be loaded.
/// \param sounds property value to split.
/// \return sample filenames in the order they were written.
std::vector<std::string> parse(const std::string& sounds);

}  // namespace SoundList

/// \brief keeps exactly one sample out of a list audible and rotates through the list.
///
/// A single sample is looped. With more than one, a random sample is played and replaced by another
/// random one as soon as it has played through, never picking the same one twice in a row. The
/// rotation is driven by the sample durations rather than by polling the mixer, so it also works for
/// owners whose update is skipped while they are far away from the player.
///
/// Owners that only need one looped sample can pass a single-entry list and simply never call
/// update(); owners that fire independent one-shots - a thunder clap, say - are a different shape and
/// should keep calling Audio::playSample directly, because this class stops the previous sample
/// whenever it starts the next one.
class SoundRotation
{
public:
   /// \brief stores the samples to rotate through and preloads them.
   /// \param samples sample filenames relative to the sfx directory.
   void setSamples(const std::vector<std::string>& samples);

   /// \brief checks whether any sample is configured.
   /// \return true when no sample was set, i.e. the owner is silent.
   bool empty() const;

   /// \brief checks whether a sample is currently occupying a sound thread.
   /// \return true between start() and stop().
   bool isPlaying() const;

   /// \brief starts the rotation if it is not running yet.
   /// \param volume volume to play the sample at.
   void start(float volume);

   /// \brief advances the rotation, starting the next sample once the current one has played through.
   /// \param dt elapsed frame time.
   /// \param volume volume to play the next sample at.
   void update(const sf::Time& dt, float volume);

   /// \brief stops the sample that is currently playing, if any.
   void stop();

   /// \brief applies a new volume to the sample that is currently playing.
   /// \param volume volume to apply.
   void setVolume(float volume);

   /// \brief returns how loud the currently playing sample is at its playback position.
   /// \return normalized loudness in 0..1, or std::nullopt when nothing is audible.
   std::optional<float> getLoudness() const;

private:
   /// \brief picks the next sample and starts playing it.
   /// \param volume volume to play the sample at.
   void playNext(float volume);

   std::vector<std::string> _samples;
   std::optional<int32_t> _thread_id;
   std::optional<size_t> _current_index;
   float _current_duration_s{0.0f};
   float _elapsed_in_current_s{0.0f};
};

#endif  // SOUNDROTATION_H
