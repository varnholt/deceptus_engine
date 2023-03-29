#include "splashparticle.h"

#include <cmath>

SplashParticle::SplashParticle(const sf::Vector2f& position, const sf::Vector2f& velocity, float orientation)
{
   _position = position;
   _velocity = velocity;
   _orientation = orientation;
}

void SplashParticle::update()
{
   constexpr auto gravity = 0.3f;
   _velocity.y += gravity;
   _position += _velocity;
   _orientation = std::atan2f(_velocity.y, _velocity.x);
}
