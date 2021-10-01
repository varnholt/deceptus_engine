#pragma once

#include "extra.h"

#include <cstdint>

#include "json/json.hpp"


class ExtraSkill : public Extra
{
public:
   ExtraSkill() = default;

   enum class Skill
   {
      WallClimb    = 0x01,
      Dash         = 0x02,
      Invulnerable = 0x04,
      WallSlide    = 0x08,
      WallJump     = 0x10,
      DoubleJump   = 0x20,
      Crouch       = 0x40,
      Swim         = 0x80
   };

   int32_t _skills = 0;
};


void to_json(nlohmann::json& j, const ExtraSkill& d);
void from_json(const nlohmann::json& j, ExtraSkill& d);

