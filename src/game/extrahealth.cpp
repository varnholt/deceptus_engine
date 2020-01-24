#include "extrahealth.h"

using json = nlohmann::json;


ExtraHealth::ExtraHealth()
{
   mExtraType = ExtraType::Health;
}


void ExtraHealth::reset()
{
   mHealth = 100;
}


void ExtraHealth::addHealth(int32_t health)
{
   mHealth += health;
   if (mHealth > mHealthMax)
   {
      mHealth = mHealthMax;
   }
}


void to_json(nlohmann::json& j, const ExtraHealth& d)
{
   j = json{
      {"lives", d.mLives},
      {"health", d.mHealth},
      {"health_max", d.mHealthMax},
   };
}


void from_json(const nlohmann::json& j, ExtraHealth& d)
{
   d.mLives = j.at("lives").get<int32_t>();
   d.mHealth = j.at("health").get<int32_t>();
   d.mHealthMax = j.at("health_max").get<int32_t>();
}

