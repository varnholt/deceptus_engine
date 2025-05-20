#include "parallaxlayer.h"
#include "framework/tmxparser/tmxlayer.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"

void ParallaxLayer::updateView(float level_view_x, float level_view_y, float view_width, float view_height)
{
   const auto x_px = level_view_x * _settings._factor.x + _settings._error.x;
   const auto y_px = level_view_y * _settings._factor.y + _settings._error.y;

   _view = sf::View(sf::FloatRect{{x_px, y_px}, {view_width, view_height}});
}

void ParallaxLayer::resetView(float view_width, float view_height)
{
   _view = sf::View{sf::FloatRect{{0.0f, 0.0f}, {view_width, view_height}}};
   _view.setViewport(sf::FloatRect{{0.0f, 0.0f}, {1.0f, 1.0f}});
}

std::unique_ptr<ParallaxLayer> ParallaxLayer::deserialize(const std::shared_ptr<TmxLayer>& layer, const std::shared_ptr<TileMap>& tile_map)
{
   if (!layer->_properties)
   {
      return nullptr;
   }

   auto parallax_layer = std::make_unique<ParallaxLayer>();

   parallax_layer->_settings.deserialize(layer->_properties);
   parallax_layer->_tile_map = tile_map;

   auto& map = layer->_properties->_map;
   const auto& it_z_index_value = map.find("z");
   if (it_z_index_value != map.end())
   {
      parallax_layer->_z_index = it_z_index_value->second->_value_int.value();
   }

   return parallax_layer;
}
