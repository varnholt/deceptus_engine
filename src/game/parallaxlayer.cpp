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

   auto z_index = 0;
   auto parallax_factor_x = 1.0f;
   auto parallax_factor_y = 1.0f;
   auto parallax_offset_x = 0.0f;
   auto parallax_offset_y = 0.0f;
   auto& map = layer->_properties->_map;

   const auto& it_parallax_x_value = map.find("factor_x");
   if (it_parallax_x_value != map.end())
   {
      parallax_factor_x = it_parallax_x_value->second->_value_float.value();
   }

   const auto& it_parallax_y_value = map.find("factor_y");
   if (it_parallax_y_value != map.end())
   {
      parallax_factor_y = it_parallax_y_value->second->_value_float.value();
   }

   const auto& it_offset_x_value = map.find("offset_x_px");
   if (it_offset_x_value != map.end())
   {
      parallax_offset_x = static_cast<float>(it_offset_x_value->second->_value_int.value());
   }

   const auto& it_offset_y_value = map.find("offset_y_px");
   if (it_offset_y_value != map.end())
   {
      parallax_offset_y = static_cast<float>(it_offset_y_value->second->_value_int.value());
   }

   const auto& it_z_index_value = map.find("z");
   if (it_z_index_value != map.end())
   {
      z_index = it_z_index_value->second->_value_int.value();
   }

   auto parallax_layer = std::make_unique<ParallaxLayer>();

   // set up parallax layer with given properties
   parallax_layer->_z_index = z_index;
   parallax_layer->_factor.x = parallax_factor_x;
   parallax_layer->_factor.y = parallax_factor_y;
   parallax_layer->_offset.x = parallax_offset_x;
   parallax_layer->_offset.y = parallax_offset_y;
   parallax_layer->_tile_map = tile_map;

   // determine placement error
   //
   //  +------------------------------------+-------+-------+
   //  |                                    |xxxxxxx|       |
   //  |                                    |xxxxxxx|       |
   //  |                                    |xxxxxxx|       |
   //  +------------------------------------+-------+-------+
   // 0px                                 800px   900px 1000px
   //
   //  800px   *   0.9     = 720px
   //  offset      factor  = actual
   //
   //  800px   -   720px   = 80px error
   //  offset      actual  = error

   const auto& parallax_factor = parallax_layer->_factor;
   auto parallax_offset_with_error = parallax_layer->_offset;
   parallax_offset_with_error.x *= parallax_factor.x;
   parallax_offset_with_error.y *= parallax_factor.y;
   parallax_layer->_error = parallax_layer->_offset - parallax_offset_with_error;

   return parallax_layer;
}
