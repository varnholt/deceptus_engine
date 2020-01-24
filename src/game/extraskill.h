#pragma once

#include "extra.h"

#include <cstdint>

#include "json/json.hpp"


class ExtraSkill : public Extra
{
public:
   ExtraSkill() = default;

   enum Skill
   {
      SkillClimb = 0x01,
      SkillInvulnerable = 0x02
   };

   int32_t mSkills = 0;
};


void to_json(nlohmann::json& j, const ExtraSkill& d);
void from_json(const nlohmann::json& j, ExtraSkill& d);

