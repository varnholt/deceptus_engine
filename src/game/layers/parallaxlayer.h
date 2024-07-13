#ifndef PARALLAXLAYER_H
#define PARALLAXLAYER_H

#include <SFML/Graphics.hpp>
#include "game/layers/parallaxsettings.h"
#include "game/level/tilemap.h"

// parallax (move to separate mechanism!)
struct ParallaxLayer
{
   static std::unique_ptr<ParallaxLayer> deserialize(const std::shared_ptr<TmxLayer>& layer, const std::shared_ptr<TileMap>& tile_map);
   void updateView(float level_view_x, float level_view_y, float view_width, float view_height);
   void resetView(float view_width, float view_height);

   int32_t _z_index = 0;
   ParallaxSettings _settings;
   std::shared_ptr<TileMap> _tile_map;
   sf::View _view;
};

#endif  // PARALLAXLAYER_H
