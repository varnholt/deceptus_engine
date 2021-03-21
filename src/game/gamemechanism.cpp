#include "gamemechanism.h"


int32_t GameMechanism::getZ() const
{
   return mZ;
}


void GameMechanism::setZ(const int32_t& z)
{
   mZ = z;
}


void GameMechanism::draw(sf::RenderTarget& /*target*/, sf::RenderTarget& /*normal*/)
{
}


void GameMechanism::update(const sf::Time& /*dt*/)
{
}


bool GameMechanism::isEnabled() const
{
   return mEnabled;
}


void GameMechanism::setEnabled(bool enabled)
{
   mEnabled = enabled;
}
