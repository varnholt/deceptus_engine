#pragma once

#include "json/json.hpp"

/// \brief string key-value pair used for script-facing properties.
struct ScriptProperty
{
   ScriptProperty() = default;

   std::string _name;
   std::string _value;
};

/// \brief serializes a ScriptProperty into a json object with name and value fields.
/// \param j destination json object.
/// \param p source property to serialize.
void to_json(nlohmann::json& j, const ScriptProperty& p);

/// \brief deserializes a ScriptProperty from a json object.
/// \param j source json object containing name and value fields.
/// \param p destination property to fill.
void from_json(const nlohmann::json& j, ScriptProperty& p);
