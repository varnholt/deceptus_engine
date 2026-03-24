#ifndef PARALLAXSETTINGS_H
#define PARALLAXSETTINGS_H

#include <SFML/Graphics.hpp>
#include <memory>

struct TmxProperties;

/// \brief parallax factors and offsets parsed from tmx layer properties.
struct ParallaxSettings
{
   sf::Vector2f _factor{1.0f, 1.0f};
   sf::Vector2f _offset{0.0f, 0.0f};
   sf::Vector2f _error{0.0f, 0.0f};
   /// \brief reads parallax factors and offsets, then precomputes placement correction.
   /// \param properties tmx property map containing factor_x, factor_y, and optional offsets.
   void deserialize(const std::shared_ptr<TmxProperties>& properties);
};

#endif  // PARALLAXSETTINGS_H
