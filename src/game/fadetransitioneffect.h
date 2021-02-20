#pragma once

#include <array>

#include "screentransitioneffect.h"

struct FadeTransitionEffect : public ScreenTransitionEffect
{
   enum class Direction {
      FadeIn,
      FadeOut
   };

   FadeTransitionEffect(const sf::Color color = sf::Color::Black);

   void update(const sf::Time& dt) override;
   void draw(const std::shared_ptr<sf::RenderTexture>& window) override;

   Direction _direction = Direction::FadeOut;
   sf::Color _fade_color = sf::Color::Black;
   float _value = 0.0f;
   float _speed = 1.0f;
   std::array<sf::Vertex, 4> _vertices;
};

