#pragma once

#include "json/json.hpp"

#include "enemydescription.h"

/// \brief json-backed description of a level file, start position, and enemy list.
struct LevelDescription
{
   /// \brief creates an empty level description.
   LevelDescription() = default;

   std::string _filename;
   std::vector<int32_t> _start_position;
   std::vector<EnemyDescription> _enemies;

   /// \brief reads and parses a level description json file.
   /// \param path filesystem path to the level description json file.
   /// \return parsed level description, or nullptr when the file is missing or invalid.
   static std::shared_ptr<LevelDescription> load(const std::string& path);
};

/// \brief serializes a level description to json.
/// \param j json object that receives the serialized fields.
/// \param d level description to serialize.
void to_json(nlohmann::json& j, const LevelDescription& d);

/// \brief deserializes a level description from json.
/// \param j json object containing level metadata and enemy definitions.
/// \param d level description to populate.
void from_json(const nlohmann::json& j, LevelDescription& d);
