#ifndef PLAYERDIVE_H
#define PLAYERDIVE_H

#include <SFML/Graphics.hpp>

class PlayerDive
{
public:
   PlayerDive() = default;

   void update(const sf::Time& dt, bool in_water);

   using HighResDuration = std::chrono::high_resolution_clock::duration;

   HighResDuration _dive_duration;
};

#endif  // PLAYERDIVE_H
