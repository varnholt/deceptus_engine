#ifndef SPLASHPARTICLE_H
#define SPLASHPARTICLE_H

#include "fakesfml.h"

struct SplashParticle
{
   SplashParticle() = default;
   SplashParticle(const sf::Vector2f& position, const sf::Vector2f& velocity, float orientation);
   void update();

   sf::Vector2f _position;
   sf::Vector2f _velocity;
   float _orientation{0.0f};
};

#endif  // SPLASHPARTICLE_H
