#include "controllerkeymap.h"

std::pair<int32_t, int32_t> ControllerKeyMap::getArrayPosition(const std::string& key)
{
   const auto it = std::find(key_map.begin(), key_map.end(), key);
   const auto index = static_cast<int32_t>(std::distance(key_map.begin(), it));

   const auto row = index / 16;
   const auto col = index % 16;

   return {col, row};
}

std::optional<std::tuple<std::string, ControllerKeyMap::InputType>> ControllerKeyMap::retrieveMappedKey(const std::string& key)
{
   const auto key_it = key_controller_map.find(key);
   if (key_it == key_controller_map.cend())
   {
      return std::tuple<std::string, ControllerKeyMap::InputType>{key, ControllerKeyMap::InputType::Controller};
   }

   return std::tuple<std::string, ControllerKeyMap::InputType>{key_it->second, ControllerKeyMap::InputType::Keyboard};
}
