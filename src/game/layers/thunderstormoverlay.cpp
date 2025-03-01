#include "thunderstormoverlay.h"

#include "framework/math/fbm.h"
#include "framework/tmxparser/tmxobject.h"

#include <iostream>

void ThunderstormOverlay::draw(sf::RenderTarget& target, sf::RenderTarget& /*normal*/)
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

   sf::RenderStates states;
   states.blendMode = sf::BlendAlpha;
   target.draw(quad, 6, sf::PrimitiveType::Triangles, states);
}

void ThunderstormOverlay::update(const sf::Time& dt)
{
   _time_s += dt.asSeconds();

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
      }
   }
}

void ThunderstormOverlay::setRect(const sf::FloatRect& rect)
{
   _rect = rect;
}

void ThunderstormOverlay::setSettings(const ThunderstormSettings& settings)
{
   _settings = settings;
}
