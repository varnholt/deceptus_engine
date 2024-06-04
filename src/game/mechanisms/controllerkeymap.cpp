#include "controllerkeymap.h"

std::pair<int32_t, int32_t> ControllerKeyMap::getArrayPosition(const std::string& key)
{
   const auto it = std::find(key_map.begin(), key_map.end(), key);
   const auto index = static_cast<int32_t>(std::distance(key_map.begin(), it));

   const auto row = index / 16;
   const auto col = index % 16;

   return {col, row};
}
