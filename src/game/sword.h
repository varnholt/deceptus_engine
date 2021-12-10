#pragma once

#include "Box2D/Box2D.h"

#include "weapon.h"

class Sword : public Weapon
{
   public:

      Sword();

      void draw(sf::RenderTarget& target) override;
      void update(const sf::Time& time) override;
      void initialize() override;

      void useNow(const std::shared_ptr<b2World>& world, const b2Vec2& pos, const b2Vec2& dir);
};

