#include "parallaxlayer.h"
#include "framework/tmxparser/tmxlayer.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"

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
