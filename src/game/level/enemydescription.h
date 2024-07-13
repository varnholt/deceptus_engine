#pragma once

#include "json/json.hpp"

#include "scriptproperty.h"

#include <cstdint>


struct EnemyDescription
{
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


void to_json(nlohmann::json& j, const EnemyDescription& d);
void from_json(const nlohmann::json& j, EnemyDescription& d);

