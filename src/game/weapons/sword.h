#pragma once

#include <chrono>
#include <vector>

#include <box2d/box2d.h>

#include "game/animation/animationpool.h"
#include "weapon.h"

class Animation;

class Sword : public Weapon
{
public:
   Sword();

   void draw(sf::RenderTarget& target) override;
   void update(const WeaponUpdateData& data) override;
   int32_t getDamage() const override;
   std::string getName() const override;

   void use(const std::shared_ptr<b2World>& world, const b2Vec2& dir);

private:
   bool checkHitWindowActive() const;
   void updateHitbox();
   void cameraShake();

   b2Vec2 _pos_m;
   b2Vec2 _dir_m;

   using HighResTimePoint = std::chrono::high_resolution_clock::time_point;
   using HighResDuration = std::chrono::high_resolution_clock::duration;

   HighResTimePoint _timepoint_swing_start;
   HighResDuration _duration_from_swing_start_to_hit;
   HighResDuration _duration_from_hit_start_to_end;

   bool _cleared_to_attack{true};
   sf::FloatRect _hit_rect_px;

   std::vector<sf::FloatRect> _octree_rects;

   AnimationPool _animation_pool{"data/sprites/weapon_animations.json"};
   std::vector<std::shared_ptr<Animation>> _animations;
   int32_t _attack_frame{0};
};
