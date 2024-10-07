#pragma once

#include <cstdint>

#include <SFML/Graphics.hpp>

#include "json/json.hpp"

struct Health
{
   Health() = default;

   enum class StaminaDrain
   {
      None = 0x0,
      Dash = 0x1
   };

   void reset();

   void addHealth(int32_t health);
   void update(const sf::Time& dt);
   void addStaminaDrain(StaminaDrain);
   void removeStaminaDrain(StaminaDrain);
   bool hasFullStamina() const;

   int32_t _life_count = 1;

   int32_t _health = 4;
   int32_t _health_max = 12;

   float _stamina{1.0f};
   int32_t _stamina_drains{0};
};

void to_json(nlohmann::json& j, const Health& d);
void from_json(const nlohmann::json& j, Health& d);
