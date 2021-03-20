#include "gamemechanism.h"


int32_t GameMechanism::getZ() const
{
   return mZ;
}


void GameMechanism::setZ(const int32_t& z)
{
   mZ = z;
}


DrawMode GameMechanism::getDrawMode() const
{
   return _draw_mode;
}


void GameMechanism::setDrawMode(DrawMode draw_mode)
{
   _draw_mode = draw_mode;
}


void GameMechanism::draw(sf::RenderTarget& /*target*/)
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
