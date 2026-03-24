#pragma once

#include "tmxelement.h"

#include <cstdint>
#include <optional>

///
/// \brief Represents one typed TMX property value.
///
struct TmxProperty : TmxElement
{
   ///
   /// \brief Constructs an empty property.
   ///
   TmxProperty() = default;

   ///
   /// \brief Deserializes property type and value from XML attributes.
   /// \param e XML element for `<property>`.
   /// \param parse_data Shared TMX parse context.
   ///
   void deserialize(tinyxml2::XMLElement* e, const std::shared_ptr<TmxParseData>& parse_data) override;

   ///
   /// \brief Converts the currently active typed value to text.
   /// \return Property value as string.
   ///
   std::string toString() const;

   std::string _value_type;

   std::optional<std::string> _value_string;
   std::optional<float> _value_float = 0.0f;
   std::optional<int32_t> _value_int = 0;
   std::optional<bool> _value_bool = false;
};
