#ifndef PARALLAXLAYER_H
#define PARALLAXLAYER_H

#include <SFML/Graphics.hpp>
#include "game/parallaxsettings.h"
#include "game/tilemap.h"

// parallax (move to separate mechanism!)
struct ParallaxLayer
{
   int32_t _z_index = 0;
   ParallaxSettings _settings;
   std::shared_ptr<TileMap> _tile_map;
   sf::View _view;

   static std::unique_ptr<ParallaxLayer> deserialize(const std::shared_ptr<TmxLayer>& layer, const std::shared_ptr<TileMap>& tile_map);
};

#endif  // PARALLAXLAYER_H
