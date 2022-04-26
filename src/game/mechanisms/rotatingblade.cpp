#include "rotatingblade.h"

namespace
{
static constexpr auto blade_acceleration = 0.003f;
static constexpr auto blade_deceleration = 0.003f;
static constexpr auto blade_rotation_speed = 0.1f;
}


RotatingBlade::RotatingBlade(GameNode* parent)
 : GameNode(parent)
{
   setClassName(typeid(RotatingBlade).name());
}


void RotatingBlade::setup(const GameDeserializeData& /*data*/)
{

}


void RotatingBlade::update(const sf::Time& dt)
{
  if (_enabled)
  {
     _velocity = std::min<float>(1.0f, _velocity + blade_acceleration);
  }
  else
  {
     _velocity = std::max<float>(0.0f, _velocity - blade_deceleration);
  }

  _angle += dt.asSeconds() * _velocity * _direction * blade_rotation_speed;
}


void RotatingBlade::draw(sf::RenderTarget& target, sf::RenderTarget& /*normal*/)
{
   target.draw(_sprite);
}


void RotatingBlade::setEnabled(bool /*enabled*/)
{

}
