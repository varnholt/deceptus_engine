#pragma once

#include "health.h"
#include "skill.h"

#include <memory>
#include <vector>

#include "json/json.hpp"


class ExtraTable
{
public:
   ExtraTable() = default;

   Health _health;
   Skill _skills;
};

void to_json(nlohmann::json& j, const ExtraTable& d);
void from_json(const nlohmann::json& j, ExtraTable& d);

