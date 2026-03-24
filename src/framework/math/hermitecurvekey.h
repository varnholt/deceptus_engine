#pragma once

#include <SFML/Graphics.hpp>

///
/// \brief Represents one timed key point for Hermite curve interpolation.
///
struct HermiteCurveKey
{
   float _time = 0.0f;
   sf::Vector2f _position;
};
