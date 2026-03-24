#pragma once

#include "json/json.hpp"

#include "scriptproperty.h"

#include <cstdint>

/// \brief serializable enemy spawn description loaded from level json data.
struct EnemyDescription
{
   /// \brief creates an empty enemy description.
   EnemyDescription() = default;

   std::string _id;
   std::string _name;
   std::string _script;
   std::vector<int32_t> _start_position;
   std::vector<int32_t> _path;
   std::vector<ScriptProperty> _properties;
   bool _position_in_tiles = true;
   bool _generate_path = false;
};

/// \brief serializes an enemy description to json.
/// \param j json object that receives the serialized fields.
/// \param d enemy description to serialize.
void to_json(nlohmann::json& j, const EnemyDescription& d);

/// \brief deserializes an enemy description from json.
/// \param j json object containing enemy spawn configuration.
/// \param d enemy description to populate.
void from_json(const nlohmann::json& j, EnemyDescription& d);
