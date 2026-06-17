#include "parallaxsettings.h"
#include "framework/tmxparser/tmxproperties.h"
#include "game/io/valuereader.h"

void ParallaxSettings::deserialize(const std::shared_ptr<TmxProperties>& properties, int32_t layer_offset_x_px, int32_t layer_offset_y_px)
{
   _offset.x = static_cast<float>(layer_offset_x_px);
   _offset.y = static_cast<float>(layer_offset_y_px);

   if (properties)
   {
      const auto& map = properties->_map;

      if (const auto factor_x = ValueReader::readValue<float>("factor_x", map))
      {
         _factor.x = *factor_x;
      }

      if (const auto factor_y = ValueReader::readValue<float>("factor_y", map))
      {
         _factor.y = *factor_y;
      }

      if (const auto offset_x_px = ValueReader::readValue<int32_t>("offset_x_px", map))
      {
         _offset.x += static_cast<float>(*offset_x_px);
      }

      if (const auto offset_y_px = ValueReader::readValue<int32_t>("offset_y_px", map))
      {
         _offset.y += static_cast<float>(*offset_y_px);
      }
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
