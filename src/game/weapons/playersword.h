#pragma once

#include <chrono>
#include <unordered_set>
#include <vector>

#include <box2d/box2d.h>

#include "game/animation/animationpool.h"
#include "game/physics/worldquery.h"
#include "weapon.h"

class Animation;

///
/// \brief The PlayerSword class implements a sword for the player, and is meant to only be passed to the player instance
///
class PlayerSword : public Weapon
{
public:
   PlayerSword();

   void draw(sf::RenderTarget& target) override;
   void update(const WeaponUpdateData& data) override;
   int32_t getDamage() const override;
   std::string getName() const override;

   void use(const std::shared_ptr<b2World>& world, const b2Vec2& dir);

private:
   bool checkHitWindowActive() const;
   void updateAnimations(const WeaponUpdateData& data);
   void updateImpact(const WeaponUpdateData& data);
   void updateHitbox();
   void updateAttackDash(const sf::Time& dt);
   void cameraShake();

   std::vector<WorldQuery::CollidedNode> impactLuaNode(std::unordered_set<b2Body*>& ignored_bodies);
   std::vector<WorldQuery::CollidedNode> impactMechanisms(std::unordered_set<b2Body*>& ignored_bodies);
   std::optional<sf::Vector2f> impactSolidObjects(const WeaponUpdateData& data, std::unordered_set<b2Body*>& ignored_bodies);

   b2Vec2 _pos_m;
   b2Vec2 _dir_m;
   bool _points_left{false};

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

   std::vector<std::pair<sf::Vector2f, sf::Vector2f>> _rays;
};
