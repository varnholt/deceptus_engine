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
   /// \param layer_offset_x_px built-in x offset of the tmx layer, used as the base parallax anchor.
   /// \param layer_offset_y_px built-in y offset of the tmx layer, used as the base parallax anchor.
   void deserialize(const std::shared_ptr<TmxProperties>& properties, int32_t layer_offset_x_px = 0, int32_t layer_offset_y_px = 0);
};

#endif  // PARALLAXSETTINGS_H
