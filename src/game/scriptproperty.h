#pragma once

#include "json/json.hpp"


struct ScriptProperty
{
   ScriptProperty() = default;

   std::string mName;
   std::string mValue;
};


void to_json(nlohmann::json& j, const ScriptProperty& p);
void from_json(const nlohmann::json& j, ScriptProperty& p);

