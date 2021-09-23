#include "extraskill.h"

using json = nlohmann::json;


void to_json(nlohmann::json& j, const ExtraSkill& d)
{
   j = json{
      {"skills", d._skills},
   };
}


void from_json(const nlohmann::json& j, ExtraSkill& d)
{
   d._skills = j.at("skills").get<int32_t>();
}

