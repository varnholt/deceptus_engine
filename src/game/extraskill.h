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
      SkillWallClimb    = 0x01,
      SkillDash         = 0x02,
      SkillInvulnerable = 0x04,
      SkillWallSlide    = 0x08,
      SkillWallJump     = 0x10,
      SkillDoubleJump   = 0x20,
      SkillCrouch       = 0x40,
      SkillSwim         = 0x80
   };

   int32_t _skills = 0;
};


void to_json(nlohmann::json& j, const ExtraSkill& d);
void from_json(const nlohmann::json& j, ExtraSkill& d);

