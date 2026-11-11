#pragma once

#if defined(DEVELOPMENT_MODE) && !defined(DECEPTUS_VRSFML)

#include <SFML/Graphics/Color.hpp>
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

/// \brief owns development-only typed adapters around ordinary runtime fields.
/// \note accessed only by editor operations; it is not part of the gameplay update path.
class MechanismPropertyBindings
{
public:
   /// \brief binds a member directly when assignment is sufficient to update runtime behavior.
   /// \tparam T supported scalar, string, or sf::Color representation.
   /// \param name authored property name; rebinding replaces the previous adapter.
   /// \param field runtime member that must outlive this registry.
   /// \param valid optional side-effect-free constraint evaluated before mutation.
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

   /// \brief binds a getter and an apply callback, with optional validation and lifetime checks.
   /// \tparam T supported scalar, string, or sf::Color representation.
   /// \param name authored property name; rebinding replaces the previous adapter.
   /// \param get reads the current runtime value without changing gameplay state.
   /// \param set applies an accepted value; false must leave its target unchanged.
   /// \param valid optional side-effect-free value constraint.
   /// \param available optional lifetime predicate; false makes the binding unavailable.
   template <typename T>
   void bind(
      const std::string& name,
      std::function<T()> get,
      std::function<bool(const T&)> set,
      std::function<bool(const T&)> valid = {},
      std::function<bool()> available = {}
   )
   {
      static_assert(std::is_same_v<T, bool> || std::is_same_v<T, int32_t> || std::is_same_v<T, float> || std::is_same_v<T, std::string> || std::is_same_v<T, sf::Color>);
      const auto validate = [valid](const std::string& text)
      {
         const auto value = parse<T>(text);
         return value && (!valid || valid(*value));
      };
      _properties.insert_or_assign(
         name,
         Property{
            [get] { return format(get()); },
            validate,
            [set, validate](const std::string& text) { return validate(text) && set(*parse<T>(text)); },
            std::move(available)
         }
      );
   }

   /// \brief validates authored text against the same schema rules used by live edits and TMX writes.
   /// \param schema definition providing the type, required flag, and allowed values.
   /// \param text candidate authored value.
   /// \return true when the supported type, required flag, and enum constraints accept the value.
   [[nodiscard]] static bool validateValue(const PropertyInfo& schema, const std::string& text)
   {
      if (schema.required && text.empty())
      {
         return false;
      }
      if (!schema.allowed_values.empty() && std::ranges::find(schema.allowed_values, text) == schema.allowed_values.end())
      {
         return false;
      }
      if (schema.type == "bool")
      {
         return parse<bool>(text).has_value();
      }
      if (schema.type == "int")
      {
         return parse<int32_t>(text).has_value();
      }
      if (schema.type == "float")
      {
         return parse<float>(text).has_value();
      }
      if (schema.type == "color")
      {
         return parseColor(text).has_value();
      }
      return schema.type == "string" && parse<std::string>(text).has_value();
   }

   /// \brief parses a Tiled alpha-first color or an RGB color with implicit opaque alpha.
   /// \param text color encoded as \c \#AARRGGBB or \c \#RRGGBB.
   /// \return RGBA channels, or null when the input is malformed.
   [[nodiscard]] static std::optional<sf::Color> parseColor(const std::string& text)
   {
      if ((text.size() != 7 && text.size() != 9) || text.front() != '#')
      {
         return std::nullopt;
      }
      uint32_t value{};
      const auto result = std::from_chars(text.data() + 1, text.data() + text.size(), value, 16);
      if (result.ec != std::errc{} || result.ptr != text.data() + text.size())
      {
         return std::nullopt;
      }
      if (text.size() == 7)
      {
         value |= 0xff000000;
      }
      return sf::Color{
         static_cast<uint8_t>(value >> 16), static_cast<uint8_t>(value >> 8), static_cast<uint8_t>(value), static_cast<uint8_t>(value >> 24)
      };
   }

   /// \brief converts runtime RGBA channels to canonical Tiled color text.
   /// \param color channels to encode, including alpha.
   /// \return lowercase \c \#AARRGGBB text.
   [[nodiscard]] static std::string formatColor(sf::Color color)
   {
      // TMX stores alpha first, while sf::Color and the widget expose RGBA
      return std::format("#{:02x}{:02x}{:02x}{:02x}", color.a, color.r, color.g, color.b);
   }

   /// \brief checks whether a named adapter exists and its target is available.
   /// \param name authored property name.
   /// \return true when the property can currently be accessed.
   [[nodiscard]] bool supports(const std::string& name) const
   {
      return find(name) != nullptr;
   }

   /// \brief reads a bound value using its canonical textual encoding.
   /// \param name authored property name.
   /// \return current value, or null when the property is unbound or unavailable.
   [[nodiscard]] std::optional<std::string> get(const std::string& name) const
   {
      const auto* property = find(name);
      return property ? std::optional{property->get()} : std::nullopt;
   }

   /// \brief checks parsing and runtime constraints without invoking a setter.
   /// \param name authored property name.
   /// \param value candidate text in the bound type's representation.
   /// \return true when an available binding accepts the candidate value.
   [[nodiscard]] bool validate(const std::string& name, const std::string& value) const
   {
      const auto* property = find(name);
      return property && property->validate(value);
   }

   /// \brief validates text and invokes the available binding's apply callback.
   /// \param name authored property name.
   /// \param value candidate text in the bound type's representation.
   /// \return true when validation and application succeed; false indicates rejection or unavailability.
   bool set(const std::string& name, const std::string& value) const
   {
      const auto* property = find(name);
      return property && property->set(value);
   }

private:
   /// \brief formats a supported runtime value for editor drafts and source overrides.
   /// \tparam T supported runtime representation.
   /// \param value runtime value to encode.
   /// \return canonical text for the bound type.
   template <typename T>
   static std::string format(const T& value)
   {
      if constexpr (std::is_same_v<T, sf::Color>)
      {
         return formatColor(value);
      }
      else
      {
         return std::format("{}", value);
      }
   }

   /// \brief parses a supported runtime representation without accepting trailing input.
   /// \tparam T supported runtime representation.
   /// \param text candidate value; numeric inputs must be finite and strings cannot contain invalid control characters.
   /// \return parsed value, or null when conversion fails.
   template <typename T>
   static std::optional<T> parse(const std::string& text)
   {
      if constexpr (std::is_same_v<T, std::string>)
      {
         if (std::ranges::any_of(text, [](unsigned char c) { return c < 32 && c != '\n' && c != '\r' && c != '\t'; }))
         {
            return std::nullopt;
         }
         return text;
      }
      else if constexpr (std::is_same_v<T, sf::Color>)
      {
         return parseColor(text);
      }
      else if constexpr (std::is_same_v<T, bool>)
      {
         if (text == "true")
         {
            return true;
         }
         if (text == "false")
         {
            return false;
         }
         return std::nullopt;
      }
      else
      {
         T value{};
         const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
         if (result.ec != std::errc{} || result.ptr != text.data() + text.size())
         {
            return std::nullopt;
         }
         if constexpr (std::is_floating_point_v<T>)
         {
            if (!std::isfinite(value))
            {
               return std::nullopt;
            }
         }
         return value;
      }
   }

   /// \brief stores type-erased access, validation, and optional lifetime callbacks.
   struct Property
   {
      std::function<std::string()> get;                  //!< reads and formats the current runtime value.
      std::function<bool(const std::string&)> validate;  //!< checks text and typed constraints without mutation.
      std::function<bool(const std::string&)> set;       //!< validates and applies a runtime value.
      std::function<bool()> available;                   //!< optional predicate guarding shorter-lived targets.
   };

   /// \brief locates a property whose optional lifetime predicate still accepts access.
   /// \param name authored property name.
   /// \return non-owning adapter pointer, or null if missing or unavailable.
   const Property* find(const std::string& name) const
   {
      const auto property = _properties.find(name);
      if (property == _properties.end() || (property->second.available && !property->second.available()))
      {
         return nullptr;
      }
      return &property->second;
   }
   std::map<std::string, Property> _properties;  //!< adapters indexed by authored property name.
};

#endif
