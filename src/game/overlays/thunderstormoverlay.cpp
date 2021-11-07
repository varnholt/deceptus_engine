#include "thunderstormoverlay.h"

#include "framework/tmxparser/tmxobject.h"


ThunderstormOverlay::ThunderstormOverlay()
{
}


void ThunderstormOverlay::draw(sf::RenderTarget& target, sf::RenderTarget& /*normal*/)
{
   const auto val = static_cast<uint8_t>(_value * 255);

   const auto col = sf::Color{val, val, val, val};

   sf::Vertex quad[] = {
      sf::Vertex(sf::Vector2f(_rect.left,               _rect.top               ), col),
      sf::Vertex(sf::Vector2f(_rect.left,               _rect.top + _rect.height), col),
      sf::Vertex(sf::Vector2f(_rect.left + _rect.width, _rect.top + _rect.height), col),
      sf::Vertex(sf::Vector2f(_rect.left + _rect.width, _rect.top               ), col)
   };

   sf::RenderStates states;
//   states.shader = &_shader;
   states.blendMode = sf::BlendAlpha;

   target.draw(quad, 4, sf::Quads, states);


   // _shape.setFillColor({val, val, val, val});
   //
   // target.draw(_shape);
}


void ThunderstormOverlay::update(const sf::Time& dt)
{
   _time_s += dt.asSeconds();
   _value = (sin(_time_s) + 1.0f) * 0.5f;
}


void ThunderstormOverlay::setRect(const sf::FloatRect& rect)
{
   _rect = rect;
//   _shape.setSize({rect.width, rect.height});
//   _shape.setPosition(rect.left, rect.top);

//   _sprite.setPosition(rect.left, rect.top);
//   _sprite.setScale();
}
