#pragma once

#include <cstdint>

#include <SFML/Graphics.hpp>

#include "json/json.hpp"

struct Health
{
   Health() = default;

   void reset();

   void addHealth(int32_t health);
   void update(const sf::Time& dt);

   int32_t _life_count = 5;

   int32_t _health = 12;
   int32_t _health_max = 12;

   float _stamina{1.0f};
};

void to_json(nlohmann::json& j, const Health& d);
void from_json(const nlohmann::json& j, Health& d);
