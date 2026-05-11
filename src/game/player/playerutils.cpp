#include "playerutils.h"

#include "game/player/playerregistry.h"

sf::Vector2f PlayerUtils::getPixelPositionFloat()
{
   return PlayerRegistry::getFirst()->getPixelPositionFloat();
}
