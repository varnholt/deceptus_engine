#include "gamemechanism.h"


int32_t GameMechanism::getZ() const
{
   return _z_index;
}


void GameMechanism::setZ(const int32_t& z)
{
   _z_index = z;
}


void GameMechanism::draw(sf::RenderTarget& /*target*/, sf::RenderTarget& /*normal*/)
{
}


void GameMechanism::update(const sf::Time& /*dt*/)
{
}


bool GameMechanism::isEnabled() const
{
   return _enabled;
}


void GameMechanism::setEnabled(bool enabled)
{
   _enabled = enabled;
}
