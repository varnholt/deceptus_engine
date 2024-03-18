#ifndef SPAWNEFFECT_H
#define SPAWNEFFECT_H

#include <SFML/Graphics.hpp>

class SpawnEffect
{
public:
   SpawnEffect();

   void draw(sf::RenderTarget& target, sf::RenderTarget& normal);
   void update(const sf::Time& dt);

private:
   sf::Time _elapsed;
};

#endif  // SPAWNEFFECT_H
