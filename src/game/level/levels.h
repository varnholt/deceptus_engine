#pragma once

#include <string>
#include <vector>

#include "json/json.hpp"

/// \brief entry describing a level selectable from the level list configuration.
struct LevelItem
{
   std::string _level_name;
};

/// \brief deserializes one level list entry from json.
/// \param j json object containing a "levelname" field.
/// \param item level item to populate.
void from_json(const nlohmann::json& j, LevelItem& item);

namespace Levels
{
/// \brief returns one level entry by index, loading levels.json on first use.
/// \param index zero-based index into the cached level list.
/// \return requested level item, or a default-constructed item when out of range.
LevelItem readLevelItem(int32_t index);

/// \brief returns all level entries, loading and caching levels.json on first use.
/// \return copy of the cached level item list.
std::vector<LevelItem> readLevelItems();
};  // namespace Levels
