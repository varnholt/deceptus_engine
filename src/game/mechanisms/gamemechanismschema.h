#ifndef GAMEMECHANISMSCHEMA_H
#define GAMEMECHANISMSCHEMA_H

#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <variant>

using PropertyValue = std::variant<std::string_view, int32_t, float, bool>;

struct PropertyInfo
{
   std::string_view name;
   std::string_view type;
   PropertyValue default_value;  //!< value the engine falls back to when the property is absent
   bool required = false;
   std::span<const std::string_view> allowed_values;  //!< fixed set of accepted values, exported as a tiled enum
   std::optional<PropertyValue>
      template_value;  //!< value a newly inserted tiled object starts with, where the engine default would be inert
};

struct MechanismSchema
{
   std::string_view type_name;
   std::string_view layer_name;
   int32_t default_width = 48;
   int32_t default_height = 48;
   std::span<const PropertyInfo> properties;
   std::string_view default_polyline;  //!< tiled polyline points, set for mechanisms placed as a path instead of a rectangle
};

#endif  // GAMEMECHANISMSCHEMA_H
