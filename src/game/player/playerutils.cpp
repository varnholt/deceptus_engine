#include "playerutils.h"

#include "game/player/player.h"

sf::Vector2f PlayerUtils::getPixelPositionFloat()
{
   return Player::getCurrent()->getPixelPositionFloat();
}
