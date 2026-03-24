#pragma once

#include "health.h"
#include "skill.h"

#include <memory>
#include <vector>

#include "json/json.hpp"

/// \brief stores extra player save data that does not belong to core stats.
class ExtraTable
{
public:
   /// \brief creates an extra table with default health and skill state.
   ExtraTable() = default;

   Health _health;
   Skill _skills;
};

/// \brief serializes extra-table health and skills into json.
/// \param j json object receiving "health" and "skills".
/// \param d extra-table source data.
void to_json(nlohmann::json& j, const ExtraTable& d);
/// \brief deserializes health and skills from json into the extra table.
/// \param j json object containing serialized extra-table fields.
/// \param d extra-table target data.
void from_json(const nlohmann::json& j, ExtraTable& d);
