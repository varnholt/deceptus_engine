#pragma once

#include <chrono>
#include <unordered_set>
#include <vector>

#include "box2d/box2d.h"

#include "game/animation/animationpool.h"
#include "game/physics/worldquery.h"
#include "weapon.h"

class Animation;

/// \brief melee weapon implementation that performs timed hit-window collision checks in front of the player.
class PlayerSword : public Weapon
{
public:
   /// \brief constructs the sword and configures swing timing and impact animation pool.
   PlayerSword();

   /// \brief draws active impact animations and optional debug overlays for sword queries.
   /// \param target render target used for sword visuals.
   /// \param states render states applied while drawing (used in WASM to carry the level view).
   void draw(sf::RenderTarget& target, const sf::RenderStates& states = {}) override;

   /// \brief updates impact animations and applies hit detection while the swing window is active.
   /// \param data per-frame update context containing delta time and world.
   void update(const WeaponUpdateData& data) override;

   /// \brief returns nominal sword damage value.
   /// \return damage amount used for sword hits.
   int32_t getDamage() const override;

   /// \brief returns the weapon name used by gameplay and config code.
   /// \return string literal "sword".
   std::string getName() const override;

   /// \brief starts a sword swing by arming the hit window and storing attack direction.
   /// \param world unused world pointer kept for interface consistency.
   /// \param dir attack direction vector in world space.
   void use(const std::shared_ptr<b2World>& world, const b2Vec2& dir);

private:
   /// \brief checks whether the current time is inside the active damage window of the swing.
   /// \return true when hits should currently be processed.
   bool checkHitWindowActive() const;

   /// \brief advances and prunes one-shot impact animations.
   /// \param data per-frame update context containing delta time.
   void updateAnimations(const WeaponUpdateData& data);

   /// \brief performs all sword impact queries against nodes, mechanisms, and solid geometry.
   /// \param data per-frame update context containing world access.
   void updateImpact(const WeaponUpdateData& data);

   /// \brief recomputes the sword hit rectangle from player pose, stance, and direction.
   void updateHitbox();

   /// \brief reserved helper for attack dash behavior.
   /// \param dt frame delta time.
   void updateAttackDash(const sf::Time& dt);

   /// \brief triggers camera shake feedback for impactful sword hits.
   static void cameraShake();

   /// \brief applies sword damage to lua nodes intersecting the current hitbox.
   /// \param ignored_bodies body set extended with impacted bodies to avoid duplicate handling.
   /// \return list of collided lua nodes detected in this step.
   std::vector<WorldQuery::CollidedNode> impactLuaNode(std::unordered_set<b2Body*>& ignored_bodies);

   /// \brief applies sword damage to destructible mechanisms intersecting the hitbox.
   /// \param ignored_bodies currently ignored bodies, kept for interface symmetry with other impact paths.
   /// \return list of impacted mechanisms.
   std::vector<std::shared_ptr<GameMechanism>> impactMechanisms(std::unordered_set<b2Body*>& ignored_bodies);

   /// \brief detects closest solid-object impact position for hit feedback and animation spawn.
   /// \param data per-frame update context containing world access.
   /// \param ignored_bodies body set excluded from solid hit tests.
   /// \return impact position in pixels when a valid solid hit was found.
   std::optional<sf::Vector2f> impactSolidObjects(const WeaponUpdateData& data, std::unordered_set<b2Body*>& ignored_bodies);

   b2Vec2 _pos_m{};
   b2Vec2 _dir_m{};
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
