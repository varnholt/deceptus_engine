#pragma once

#include <chrono>

#include "Box2D/Box2D.h"

#include "weapon.h"

class Sword : public Weapon
{
   public:

      Sword();

      void draw(sf::RenderTarget& target) override;
      void update(const sf::Time& time) override;
      void initialize() override;

      void use(const std::shared_ptr<b2World>& world, const b2Vec2& dir);

   private:

      b2Vec2 _pos_m;
      b2Vec2 _dir_m;

      using HighResTimePoint = std::chrono::high_resolution_clock::time_point;
      HighResTimePoint _timepoint_used;

      bool _cleared_to_attack{true};
};

