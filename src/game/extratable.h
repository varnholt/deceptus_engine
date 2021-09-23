#pragma once

#include "extrahealth.h"
#include "extraskill.h"

#include <memory>
#include <vector>

#include "json/json.hpp"


class ExtraTable
{
public:
   ExtraTable() = default;

   ExtraHealth _health;
   ExtraSkill _skills;
};

void to_json(nlohmann::json& j, const ExtraTable& d);
void from_json(const nlohmann::json& j, ExtraTable& d);

