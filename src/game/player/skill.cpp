#include "skill.h"

using json = nlohmann::json;

void to_json(nlohmann::json& j, const Skill& d)
{
   j = json{
      {"skills", d._skills},
   };
}

void from_json(const nlohmann::json& j, Skill& d)
{
   d._skills = j.at("skills").get<int32_t>();
}
