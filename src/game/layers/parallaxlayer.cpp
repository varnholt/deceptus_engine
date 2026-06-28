#include "parallaxlayer.h"
#include "framework/tmxparser/tmxlayer.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "game/io/valuereader.h"

void ParallaxLayer::updateView(float level_view_x, float level_view_y, float view_width, float view_height)
{
   const auto x_px = level_view_x * _settings._factor.x + _settings._error.x;
   const auto y_px = level_view_y * _settings._factor.y + _settings._error.y;

   _view = sf::View::fromRect(sf::FloatRect{{x_px, y_px}, {view_width, view_height}});
}

void ParallaxLayer::resetView(float view_width, float view_height)
{
   _view = sf::View::fromRect(sf::FloatRect{{0.0f, 0.0f}, {view_width, view_height}});
   _view.viewport = sf::FloatRect{{0.0f, 0.0f}, {1.0f, 1.0f}};
}

std::unique_ptr<ParallaxLayer> ParallaxLayer::deserialize(const std::shared_ptr<TmxLayer>& layer, const std::shared_ptr<TileMap>& tile_map)
{
   auto parallax_layer = std::make_unique<ParallaxLayer>();

   parallax_layer->_settings.deserialize(layer->_properties, layer->_position_x_px, layer->_position_y_px);
   parallax_layer->_tile_map = tile_map;

   if (layer->_properties)
   {
      const auto& map = layer->_properties->_map;
      if (const auto z_index = ValueReader::readValue<int32_t>("z", map))
      {
         parallax_layer->_z_index = *z_index;
      }
   }

   return parallax_layer;
}
