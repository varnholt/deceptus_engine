#ifndef PARALLAXLAYER_H
#define PARALLAXLAYER_H

#include <SFML/Graphics.hpp>
#include "game/layers/parallaxsettings.h"
#include "game/level/tilemap.h"

// parallax (move to separate mechanism!)
/// \brief binds a tile map layer to parallax settings and an sf::View.
struct ParallaxLayer
{
   /// \brief creates a parallax layer from tmx layer properties.
   /// \param layer tmx layer that contains parallax properties.
   /// \param tile_map tile map rendered with the computed parallax view.
   /// \return initialized parallax layer, or nullptr when no properties are available.
   static std::unique_ptr<ParallaxLayer> deserialize(const std::shared_ptr<TmxLayer>& layer, const std::shared_ptr<TileMap>& tile_map);

   /// \brief updates the internal view using camera position and parallax factors.
   /// \param level_view_x level camera x-position in pixels.
   /// \param level_view_y level camera y-position in pixels.
   /// \param view_width viewport width in pixels.
   /// \param view_height viewport height in pixels.
   void updateView(float level_view_x, float level_view_y, float view_width, float view_height);

   /// \brief resets the internal view to origin with a full-screen viewport.
   /// \param view_width viewport width in pixels.
   /// \param view_height viewport height in pixels.
   void resetView(float view_width, float view_height);

   int32_t _z_index = 0;
   ParallaxSettings _settings;
   std::shared_ptr<TileMap> _tile_map;
   sf::View _view;
};

#endif  // PARALLAXLAYER_H
