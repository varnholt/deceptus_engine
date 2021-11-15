#pragma once

#include <SFML/Graphics.hpp>

namespace PlayerAttack
{

struct Attack
{
   sf::FloatRect _collision_rect;
   int32_t _damage = 0;
};

void attack(const Attack&);

}

