#pragma once

#ifdef DECEPTUS_MECHANISM_INSPECTOR

#include "game/mechanisms/gamemechanismschema.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <format>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

// developer-only adapters around ordinary runtime fields; never used by the gameplay update path
class MechanismPropertyBindings
{
public:
   template <typename T>
   void bind(const std::string& name, T& field, std::type_identity_t<std::function<bool(const T&)>> valid = {})
   {
      bind<T>(
         name,
         [&field] { return field; },
         [&field](const T& value)
         {
            field = value;
            return true;
         },
         std::move(valid)
      );
   }

   template <typename T>
   void bind(const std::string& name, std::function<T()> get, std::function<bool(const T&)> set, std::function<bool(const T&)> valid = {})
   {
      static_assert(std::is_same_v<T, bool> || std::is_same_v<T, int32_t> || std::is_same_v<T, float> || std::is_same_v<T, std::string>);
      const auto validate = [valid](const std::string& text)
      {
         const auto value = parse<T>(text);
         return value && (!valid || valid(*value));
      };
      _properties.insert_or_assign(
         name,
         Property{
            [get] { return std::format("{}", get()); },
            validate,
            [set, validate](const std::string& text) { return validate(text) && set(*parse<T>(text)); }
         }
      );
   }

   // shared by live edits and TMX writing so both accept the same authored values
   [[nodiscard]] static bool validateValue(const PropertyInfo& schema, const std::string& text)
   {
      if (schema.required && text.empty())
         return false;
      if (!schema.allowed_values.empty() && std::ranges::find(schema.allowed_values, text) == schema.allowed_values.end())
         return false;
      if (schema.type == "bool")
         return parse<bool>(text).has_value();
      if (schema.type == "int")
         return parse<int32_t>(text).has_value();
      if (schema.type == "float")
         return parse<float>(text).has_value();
      return schema.type == "string" && parse<std::string>(text).has_value();
   }

   [[nodiscard]] bool supports(const std::string& name) const
   {
      return _properties.contains(name);
   }

   [[nodiscard]] std::optional<std::string> get(const std::string& name) const
   {
      const auto property = _properties.find(name);
      return property == _properties.end() ? std::nullopt : std::optional{property->second.get()};
   }

   [[nodiscard]] bool validate(const std::string& name, const std::string& value) const
   {
      const auto property = _properties.find(name);
      return property != _properties.end() && property->second.validate(value);
   }

   bool set(const std::string& name, const std::string& value) const
   {
      const auto property = _properties.find(name);
      return property != _properties.end() && property->second.set(value);
   }

private:
   template <typename T>
   static std::optional<T> parse(const std::string& text)
   {
      if constexpr (std::is_same_v<T, std::string>)
      {
         if (std::ranges::any_of(text, [](unsigned char c) { return c < 32 && c != '\n' && c != '\r' && c != '\t'; }))
            return std::nullopt;
         return text;
      }
      else if constexpr (std::is_same_v<T, bool>)
      {
         if (text == "true")
            return true;
         if (text == "false")
            return false;
         return std::nullopt;
      }
      else
      {
         T value{};
         const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
         if (result.ec != std::errc{} || result.ptr != text.data() + text.size())
            return std::nullopt;
         if constexpr (std::is_floating_point_v<T>)
            if (!std::isfinite(value))
               return std::nullopt;
         return value;
      }
   }

   struct Property
   {
      std::function<std::string()> get;
      std::function<bool(const std::string&)> validate;
      std::function<bool(const std::string&)> set;
   };
   std::map<std::string, Property> _properties;
};

#endif
