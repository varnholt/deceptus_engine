#ifndef PLAYERUTILS_H
#define PLAYERUTILS_H

#include <SFML/Graphics.hpp>

namespace PlayerUtils
{

/// \brief returns the current player position in pixel coordinates.
/// \return player world position as an sf::Vector2f in pixels.
sf::Vector2f getPixelPositionFloat();

};

#endif  // PLAYERUTILS_H
