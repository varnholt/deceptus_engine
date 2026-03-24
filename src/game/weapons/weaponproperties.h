#ifndef WEAPONPROPERTIES_H
#define WEAPONPROPERTIES_H

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>

class b2Body;
class b2Shape;

/// \brief lightweight property bag passed into weapon constructors.
struct WeaponProperties
{
   using PropertyTypes = std::variant<std::string, int32_t, float, bool>;
   /// \brief reads a typed property by key and falls back when the key is missing.
   /// \param key property name to look up.
   /// \param default_value value returned when the key is not present.
   /// \return stored property cast to T, or default_value when key is absent.
   template <class T>
   T read(const std::string& key, T default_value) const
   {
      const auto it = _properties.find(key);
      if (it != _properties.end())
      {
         return std::get<T>(it->second);
      }

      return default_value;
   }

   std::unordered_map<std::string, PropertyTypes> _properties;
   b2Body* _parent_body{nullptr};
   mutable std::unique_ptr<b2Shape> _shape;
};

#endif  // WEAPONPROPERTIES_H
