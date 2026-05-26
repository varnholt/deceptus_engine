#ifndef GAMEMECHANISMSCHEMA_H
#define GAMEMECHANISMSCHEMA_H

#include <cstdint>
#include <span>
#include <string_view>
#include <variant>

struct PropertyInfo
{
   std::string_view name;
   std::string_view type;
   std::variant<std::string_view, int32_t, float, bool> default_value;
   bool required = false;
};

struct MechanismSchema
{
   std::string_view type_name;
   std::string_view layer_name;
   int32_t default_width = 48;
   int32_t default_height = 48;
   std::span<const PropertyInfo> properties;
};

#endif  // GAMEMECHANISMSCHEMA_H
