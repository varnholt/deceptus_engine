#include "spawneffect.h"

SpawnEffect::SpawnEffect()
{
}

void SpawnEffect::draw(sf::RenderTarget& target, sf::RenderTarget& /*normal*/)
{
}

void SpawnEffect::update(const sf::Time& dt)
{
   _elapsed += dt;
}
