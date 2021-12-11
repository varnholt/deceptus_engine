#pragma once

#include <SFML/Graphics.hpp>

#include "constants.h"


class Weapon
{

public:

   Weapon() = default;
   WeaponType getWeaponType() const;

   virtual void draw(sf::RenderTarget& target);
   virtual void update(const sf::Time& time);
   virtual void initialize();

   // todo
   // make virtual and move to subclasses
   int32_t damage() const;

protected:

   WeaponType _type = WeaponType::Invalid;
};
