#include "fadetransitioneffect.h"

#include "game/config/gameconfiguration.h"

FadeTransitionEffect::FadeTransitionEffect(const sf::Color color) : _fade_color(color)
{
   const auto w = static_cast<float>(GameConfiguration::getInstance()._view_width);
   const auto h = static_cast<float>(GameConfiguration::getInstance()._view_height);

   const sf::Vector2f top_left = {0, 0};
   const sf::Vector2f bottom_left = {0, h};
   const sf::Vector2f bottom_right = {w, h};
   const sf::Vector2f top_right = {w, 0};

   _vertices = {
      sf::Vertex{top_left, _fade_color},
      sf::Vertex{bottom_left, _fade_color},
      sf::Vertex{top_right, _fade_color},
      sf::Vertex{bottom_right, _fade_color}
   };
}

void FadeTransitionEffect::update(const sf::Time& dt)
{
   if (_done)
   {
      return;
   }

   float sign = 1.0f;

   switch (_direction)
   {
      case Direction::FadeIn:
         sign = -1.0f;
         break;
      case Direction::FadeOut:
         sign = 1.0f;
         break;
   }

   _value = _value + (_speed * sign * dt.asSeconds());

   switch (_direction)
   {
      case Direction::FadeIn:
      {
         if (_value <= 0.0)
         {
            _value = 0.0f;
            done();
         }
         break;
      }
      case Direction::FadeOut:
      {
         if (_value >= 1.0)
         {
            _value = 1.0f;
            done();
         }
         break;
      }
   }
}

void FadeTransitionEffect::draw(const std::shared_ptr<sf::RenderTexture>& window)
{
   auto w = GameConfiguration::getInstance()._view_width;
   auto h = GameConfiguration::getInstance()._view_height;

   sf::View view(sf::FloatRect({0.0f, 0.0f}, {static_cast<float>(w), static_cast<float>(h)}));
   window->setView(view);

   const uint8_t alpha_value = static_cast<uint8_t>(_value * 255);
   for (auto& vertex : _vertices)
   {
      vertex.color.a = alpha_value;
   }

   window->draw(_vertices.data(), _vertices.size(), sf::PrimitiveType::TriangleStrip);
}
