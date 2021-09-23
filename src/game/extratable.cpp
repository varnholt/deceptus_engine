#include "extratable.h"

#include "extrahealth.h"

using json = nlohmann::json;


void to_json(nlohmann::json& j, const ExtraTable& d)
{
   j = json{
      {"health", d._health},
      {"skills", d._skills}
   };
}


void from_json(const nlohmann::json& j, ExtraTable& d)
{
   d._health = j.at("health").get<ExtraHealth>();
   d._skills = j.at("skills").get<ExtraSkill>();
}


