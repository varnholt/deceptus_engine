#pragma once

#include <cstdint>

#include "json/json.hpp"

class Skill
{
public:
   Skill() = default;

   enum class SkillType
   {
      WallClimb = 0x01,
      Dash = 0x02,
      Invulnerable = 0x04,
      WallSlide = 0x08,
      WallJump = 0x10,
      DoubleJump = 0x20,
      Crouch = 0x40,
      Swim = 0x80
   };

   int32_t _skills = 0;
};

void to_json(nlohmann::json& j, const Skill& d);
void from_json(const nlohmann::json& j, Skill& d);
