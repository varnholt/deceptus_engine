#pragma once

#include "extrahealth.h"
#include "extraskill.h"
#include "extrakey.h"

#include <memory>
#include <vector>

#include "json/json.hpp"


class ExtraTable
{
public:
   ExtraTable() = default;

   ExtraHealth mHealth;
   ExtraSkill mSkills;
};

void to_json(nlohmann::json& j, const ExtraTable& d);
void from_json(const nlohmann::json& j, ExtraTable& d);

