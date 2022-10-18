#ifndef WATERBUBBLES_H
#define WATERBUBBLES_H

#include "SFML/Graphics.hpp"

class WaterBubbles
{
public:
   WaterBubbles();

   void draw(sf::RenderTarget& target, sf::RenderTarget& normal);
   void update(const sf::Time& dt);
};

#endif // WATERBUBBLES_H
