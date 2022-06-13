#pragma once

#include "tmxelement.h"

#include <cstdint>
#include <optional>

struct TmxProperty : TmxElement
{
   TmxProperty() = default;

   void deserialize(tinyxml2::XMLElement* e, const std::shared_ptr<TmxParseData>&) override;

   std::string toString() const;

   std::string _value_type;

   std::optional<std::string> _value_string;
   std::optional<float> _value_float = 0.0f;
   std::optional<int32_t> _value_int = 0;
   std::optional<bool> _value_bool = false;
};

