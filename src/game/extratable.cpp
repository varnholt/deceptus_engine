#include "extratable.h"

#include "extrahealth.h"

using json = nlohmann::json;


void to_json(nlohmann::json& j, const ExtraTable& d)
{
   j = json{
      {"health", d.mHealth},
      {"skills", d.mSkills}
   };
}


void from_json(const nlohmann::json& j, ExtraTable& d)
{
   d.mHealth = j.at("health").get<ExtraHealth>();
   d.mSkills = j.at("skills").get<ExtraSkill>();
}


