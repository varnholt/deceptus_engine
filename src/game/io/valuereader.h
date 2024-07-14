#ifndef VALUEREADER_H
#define VALUEREADER_H

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>

#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"

namespace ValueReader
{

template <typename T>
std::optional<T> readValue(const std::string& key, const std::map<std::string, std::shared_ptr<TmxProperty>>& map)
{
   const auto it = map.find(key);
   if (it != map.end())
   {
      if constexpr (std::is_same_v<T, int32_t>)
      {
         return it->second->_value_int;
      }
      else if constexpr (std::is_same_v<T, float>)
      {
         return it->second->_value_float;
      }
      else if constexpr (std::is_same_v<T, std::string>)
      {
         return it->second->_value_string;
      }
      else if constexpr (std::is_same_v<T, bool>)
      {
         return it->second->_value_bool;
      }
   }
   return std::nullopt;
}
};  // namespace ValueReader

#endif  // VALUEREADER_H
