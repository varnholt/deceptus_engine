#ifndef WEAPONPROPERTIES_H
#define WEAPONPROPERTIES_H

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <variant>

class b2Body;
class b2Shape;

struct WeaponProperties
{
   using PropertyTypes = std::variant<std::string, int32_t, float, bool>;

   b2Body* _parent_body{nullptr};
   mutable std::unique_ptr<b2Shape> _shape;

   PropertyTypes read(const std::string& key, const PropertyTypes& default_value) const
   {
      const auto it = _properties.find(key);
      if (it != _properties.end())
      {
         return it->second;
      }

      return default_value;
   }

   std::unordered_map<std::string, PropertyTypes> _properties;
};

#endif // WEAPONPROPERTIES_H
