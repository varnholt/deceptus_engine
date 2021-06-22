#pragma once

#include <SFML/Graphics.hpp>

class PlayerAnimation
{

public:

   PlayerAnimation() = default;

   // put all relevant things in a struct and pass it here
   // do the same for 'PlayerJump'

   void update(const sf::Time& dt);

};

