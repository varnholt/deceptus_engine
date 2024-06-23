#include "controllerkeymap.h"

#include <algorithm>

std::pair<int32_t, int32_t> ControllerKeyMap::getArrayPosition(const std::string& key)
{
   const auto it = std::find(key_map.begin(), key_map.end(), key);
   const auto index = static_cast<int32_t>(std::distance(key_map.begin(), it));

   const auto row = index / 16;
   const auto col = index % 16;

   return {col, row};
}

std::pair<std::string, std::string> ControllerKeyMap::retrieveMappedKey(const std::string& key)
{
   const auto key_it = key_controller_map.find(key);

   // key has no controller buddy, just use same identifier for both keyboard and controller
   if (key_it == key_controller_map.cend())
   {
      return {key, key};
   }

   // return keyboard <-> controller tuple
   return {key, key_it->second};
}
