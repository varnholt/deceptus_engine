#include "playerutils.h"

#include "player/player.h"

sf::Vector2f PlayerUtils::getPixelPositionFloat()
{
   return Player::getCurrent()->getPixelPositionFloat();
}
