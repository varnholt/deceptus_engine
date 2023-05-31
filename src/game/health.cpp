#include "health.h"

#include <iostream>

using json = nlohmann::json;

void Health::reset()
{
   _health = 12;
   _stamina = 1.0f;
}

void Health::addHealth(int32_t health)
{
   _health += health;

   if (_health > _health_max)
   {
      _health = _health_max;
   }
}

void Health::update(const sf::Time& dt)
{
   // static int32_t tick = 0;
   // tick++;
   // if (tick % 10 == 0)
   // {
   //    std::cout << _stamina << std::endl;
   // }

   static constexpr auto stamina_charge_factor = 1.0f;
   static constexpr auto stamina_drain_factor = 1.0f;

   if (_stamina_drains != 0)
   {
      _stamina -= stamina_drain_factor * dt.asSeconds();
      _stamina = std::max(_stamina, 0.0f);
   }
   else
   {
      _stamina += stamina_charge_factor * dt.asSeconds();
      _stamina = std::min(_stamina, 1.0f);
   }
}

void Health::addStaminaDrain(StaminaDrain drain)
{
   _stamina_drains |= static_cast<int32_t>(drain);
}

void Health::removeStaminaDrain(StaminaDrain drain)
{
   _stamina_drains &= ~static_cast<int32_t>(drain);
}

bool Health::hasFullStamina() const
{
   return _stamina > 0.999f;
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

