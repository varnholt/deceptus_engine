#include "health.h"

using json = nlohmann::json;

void Health::reset()
{
   _health = 12;
}

void Health::addHealth(int32_t health)
{
   _health += health;

   if (_health > _health_max)
   {
      _health = _health_max;
   }
}

void to_json(nlohmann::json& j, const Health& d)
{
   j = json{
      {"lives", d._life_count},
      {"health", d._health},
      {"health_max", d._health_max},
   };
}

void from_json(const nlohmann::json& j, Health& d)
{
   d._life_count = j.at("lives").get<int32_t>();
   d._health = j.at("health").get<int32_t>();
   d._health_max = j.at("health_max").get<int32_t>();
}

