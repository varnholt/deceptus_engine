#pragma once

#include "tmxelement.h"

#include <cstdint>
#include <optional>

struct TmxProperty : TmxElement
{
   TmxProperty() = default;

   void deserialize(tinyxml2::XMLElement* e) override;

   std::string toString() const;

   std::string mValueType;
   std::optional<std::string> mValueStr;
   std::optional<float> mValueFloat = 0.0f;
   std::optional<int32_t> mValueInt = 0;
   std::optional<bool> mValueBool = false;
};

