#include "thunderstormoverlay.h"

#include "framework/math/fbm.h"
#include "framework/tmxparser/tmxobject.h"
#include "framework/tools/log.h"
#include "game/audio/audio.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>

void ThunderstormOverlay::draw(sf::RenderTarget& target, sf::RenderTarget& normal)
{
   draw(target, normal, sf::RenderStates{});
}

void ThunderstormOverlay::draw(sf::RenderTarget& target, sf::RenderTarget& /*normal*/, const sf::RenderStates& states)
{
   const auto val = static_cast<uint8_t>(_value * 255);
   const auto col = sf::Color{val, val, val, val};

   const sf::Vector2f top_left = {_rect.position.x, _rect.position.y};
   const sf::Vector2f bottom_left = {_rect.position.x, _rect.position.y + _rect.size.y};
   const sf::Vector2f bottom_right = {_rect.position.x + _rect.size.x, _rect.position.y + _rect.size.y};
   const sf::Vector2f top_right = {_rect.position.x + _rect.size.x, _rect.position.y};

   sf::Vertex quad[6] = {
      sf::Vertex(top_left, col),
      sf::Vertex(bottom_left, col),
      sf::Vertex(bottom_right, col),

      sf::Vertex(top_left, col),
      sf::Vertex(bottom_right, col),
      sf::Vertex(top_right, col)
   };

   sf::RenderStates quad_states;
   quad_states.blendMode = sf::BlendAlpha;
#ifdef DECEPTUS_VRSFML
   // the level view travels in the render states, the render target does not carry one; without it the
   // lightning quad would be placed with the target's default view and land outside the visible area
   quad_states.view = (states.view == sf::View{}) ? target.computeView() : states.view;
   target.draw(quad, sf::PrimitiveType::Triangles, quad_states);
#else
   (void)states;
   target.draw(quad, 6, sf::PrimitiveType::Triangles, quad_states);
#endif
}

void ThunderstormOverlay::update(const sf::Time& dt)
{
   _time_s += dt.asSeconds();

   // the flash has already gone off; the sound of it is still on its way
   if (_pending_thunder_s.has_value())
   {
      _pending_thunder_s = _pending_thunder_s.value() - dt.asSeconds();
      if (_pending_thunder_s.value() <= 0.0f)
      {
         _pending_thunder_s.reset();
         playThunder(_pending_thunder_sample, _pending_thunder_volume);
      }
   }

   _value = fbm::fbm({_time_s, 0.0f}) * 3.0f;

   if (_state == State::Lightning)
   {
      _thunderstorm_time_elapsed_s += dt.asSeconds();

      _factor += dt.asSeconds() * 5.0f;
      ;
      _factor = std::min(_factor, 1.0f);
      _value *= _factor;

      if (_thunderstorm_time_elapsed_s > _settings._thunderstorm_time_s)
      {
         // start silence
         _silence_time_elapsed_s = 0.0f;
         _state = State::Silence;
      }
   }

   if (_state == State::Silence)
   {
      _silence_time_elapsed_s += dt.asSeconds();

      _factor -= dt.asSeconds() * 5.0f;
      ;
      _factor = std::max(_factor, 0.0f);
      _value *= _factor;

      if (_silence_time_elapsed_s > _settings._silence_time_s)
      {
         // start lightning
         _thunderstorm_time_elapsed_s = 0.0f;
         _state = State::Lightning;
         scheduleThunder(std::nullopt, std::nullopt);
      }
   }
}

void ThunderstormOverlay::strike(const std::optional<std::string>& sample, const std::optional<float>& volume)
{
   _thunderstorm_time_elapsed_s = 0.0f;
   _state = State::Lightning;
   _factor = 1.0f;

   scheduleThunder(sample, volume);
}

void ThunderstormOverlay::scheduleThunder(const std::optional<std::string>& sample, const std::optional<float>& volume)
{
   _pending_thunder_s = _settings._thunder_delay_s;
   _pending_thunder_volume = volume;
   _pending_thunder_sample = sample;
}

void ThunderstormOverlay::playThunder(const std::optional<std::string>& sample, const std::optional<float>& volume)
{
   if (_settings._sounds.empty())
   {
      return;
   }

   // a named sample has to be one of the configured ones, those are the ones that were preloaded
   if (sample.has_value())
   {
      const auto named = std::ranges::find(_settings._sounds, sample.value());
      if (named != _settings._sounds.end())
      {
         _previous_sound_index = static_cast<size_t>(std::distance(_settings._sounds.begin(), named));
         Audio::getInstance().playSample({*named, volume.value_or(_settings._sound_volume)});
         return;
      }

      Log::Warning() << "thunder sample '" << sample.value() << "' is not one of this weather object's sounds";
   }

   auto index = size_t{0};
   if (_settings._sounds.size() > 1)
   {
      // hearing the same sample twice in a row makes the randomization look broken
      std::uniform_int_distribution<size_t> distribution{0, _settings._sounds.size() - 1};
      do
      {
         index = distribution(_random_engine);
      } while (_previous_sound_index.has_value() && index == _previous_sound_index.value());
   }

   _previous_sound_index = index;
   Audio::getInstance().playSample({_settings._sounds[index], volume.value_or(_settings._sound_volume)});
}

void ThunderstormOverlay::setRect(const sf::FloatRect& rect)
{
   _rect = rect;
}

void ThunderstormOverlay::setSettings(const ThunderstormSettings& settings)
{
   _settings = settings;
}
