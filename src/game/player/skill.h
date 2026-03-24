#pragma once

#include <cstdint>

#include "json/json.hpp"

/// \brief stores unlocked player skills as a bitmask.
class Skill
{
public:
   /// \brief creates an empty skill set with no unlocked abilities.
   Skill() = default;

   /// \brief bit flags for individually unlockable player abilities.
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

/// \brief serializes the skill bitmask to json.
/// \param j json object that receives the "skills" field.
/// \param d skill data source.
void to_json(nlohmann::json& j, const Skill& d);

/// \brief deserializes the skill bitmask from json.
/// \param j json object that contains the "skills" field.
/// \param d skill data target to populate.
void from_json(const nlohmann::json& j, Skill& d);
