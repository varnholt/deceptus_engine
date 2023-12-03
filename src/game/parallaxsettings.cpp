#include "parallaxsettings.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"

void ParallaxSettings::deserialize(const std::shared_ptr<TmxProperties>& properties)
{
   auto& map = properties->_map;

   const auto& it_parallax_x_value = map.find("factor_x");
   if (it_parallax_x_value != map.end())
   {
      _factor.x = it_parallax_x_value->second->_value_float.value();
   }

   const auto& it_parallax_y_value = map.find("factor_y");
   if (it_parallax_y_value != map.end())
   {
      _factor.y = it_parallax_y_value->second->_value_float.value();
   }

   const auto& it_offset_x_value = map.find("offset_x_px");
   if (it_offset_x_value != map.end())
   {
      _offset.x = static_cast<float>(it_offset_x_value->second->_value_int.value());
   }

   const auto& it_offset_y_value = map.find("offset_y_px");
   if (it_offset_y_value != map.end())
   {
      _offset.y = static_cast<float>(it_offset_y_value->second->_value_int.value());
   }

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

   auto parallax_offset_with_error = _offset;
   parallax_offset_with_error.x *= _factor.x;
   parallax_offset_with_error.y *= _factor.y;
   _error = _offset - parallax_offset_with_error;
}
