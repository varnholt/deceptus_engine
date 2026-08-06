#include "soundrotation.h"

#include "framework/tmxparser/tmxtools.h"
#include "game/audio/audio.h"

#include <cstdlib>

std::vector<std::string> SoundList::parse(const std::string& sounds)
{
   std::vector<std::string> result;

   for (const auto& sound : TmxTools::split(sounds, ';'))
   {
      if (!sound.empty())
      {
         result.push_back(sound);
      }
   }

   return result;
}

void SoundRotation::setSamples(const std::vector<std::string>& samples)
{
   _samples = samples;

   for (const auto& sample : _samples)
   {
      Audio::getInstance().addSample(sample);
   }
}

bool SoundRotation::empty() const
{
   return _samples.empty();
}

bool SoundRotation::isPlaying() const
{
   return _thread_id.has_value();
}

void SoundRotation::start(float volume)
{
   if (_samples.empty() || _thread_id.has_value())
   {
      return;
   }

   playNext(volume);
}

void SoundRotation::update(const sf::Time& dt, float volume)
{
   if (_samples.empty())
   {
      return;
   }

   if (!_thread_id.has_value())
   {
      playNext(volume);
      return;
   }

   // a single sample is looped, so there is nothing to advance. a duration of 0 means the sample
   // could not be measured; rotating on that would restart it every frame.
   if (_samples.size() < 2 || _current_duration_s <= 0.0f)
   {
      return;
   }

   _elapsed_in_current_s += dt.asSeconds();
   if (_elapsed_in_current_s >= _current_duration_s)
   {
      playNext(volume);
   }
}

void SoundRotation::playNext(float volume)
{
   stop();

   auto next_index = size_t{0};
   const auto looped = (_samples.size() == 1);

   if (!looped)
   {
      // hearing the same sample twice in a row makes the randomization look broken
      do
      {
         next_index = static_cast<size_t>(std::rand()) % _samples.size();
      } while (_current_index.has_value() && next_index == _current_index.value());
   }

   const auto& sample = _samples[next_index];
   const auto duration = Audio::getInstance().getSampleDuration(sample);

   _current_index = next_index;
   _elapsed_in_current_s = 0.0f;
   _current_duration_s = duration.has_value() ? duration->asSeconds() : 0.0f;
   _thread_id = Audio::getInstance().playSample({sample, volume, looped});
}

void SoundRotation::stop()
{
   if (!_thread_id.has_value())
   {
      return;
   }

   Audio::getInstance().stopSample(_thread_id.value());
   _thread_id.reset();
}

void SoundRotation::setVolume(float volume)
{
   if (!_thread_id.has_value())
   {
      return;
   }

   Audio::getInstance().setVolume(_thread_id.value(), volume);
}

std::optional<float> SoundRotation::getLoudness() const
{
   if (!_thread_id.has_value())
   {
      return std::nullopt;
   }

   return Audio::getInstance().getSampleLoudness(_thread_id.value());
}
