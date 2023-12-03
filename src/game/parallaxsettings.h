#ifndef PARALLAXSETTINGS_H
#define PARALLAXSETTINGS_H

#include <SFML/Graphics.hpp>

struct TmxProperties;

struct ParallaxSettings
{
   sf::Vector2f _factor{1.0f, 1.0f};
   sf::Vector2f _offset{0.0f, 0.0f};
   sf::Vector2f _error{0.0f, 0.0f};
   void deserialize(const std::shared_ptr<TmxProperties>& properties);
};

#endif // PARALLAXSETTINGS_H
