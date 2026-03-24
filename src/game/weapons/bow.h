#pragma once

#include "gun.h"

#include "box2d/box2d.h"

struct Arrow;

/// \brief weapon that spawns and launches arrow projectiles with bow-specific handling.
class Bow : public Gun
{
public:
   /// \brief constructs a bow and initializes projectile shape and reference animation.
   /// \param properties runtime weapon configuration, including owner body and timing values.
   Bow(const WeaponProperties& properties = _default_properties);

   /// \brief creates a loaded arrow body and projectile object without firing it yet.
   /// \param world box2d world where the temporary loaded arrow body is created.
   void load(b2World* world);
   /// \brief loads and fires an arrow from the launcher body in the requested direction.
   /// \param world box2d world that owns the spawned arrow body.
   /// \param pos launch transform position in box2d world units.
   /// \param dir launch direction vector used for angle and initial velocity.
   void use(const std::shared_ptr<b2World>& world, const b2Vec2& pos, const b2Vec2& dir) override;
   /// \brief updates gun base behavior and keeps loaded and flying arrows aligned to velocity.
   /// \param data per-frame update context containing time and world.
   void update(const WeaponUpdateData& data) override;
   /// \brief returns nominal bow damage value.
   /// \return damage amount used by bow projectiles.
   int32_t getDamage() const override;
   /// \brief returns the weapon name used by gameplay and config code.
   /// \return string literal "bow".
   std::string getName() const override;

   /// \brief returns the box2d body that acts as launch origin for arrows.
   /// \return pointer to the configured launcher body.
   b2Body* getLauncherBody() const;
   /// \brief sets the box2d body used as launch origin and orientation source.
   /// \param launcher_body owner body that fires and carries loaded arrows.
   void setLauncherBody(b2Body* launcher_body);

private:
   /// \brief rotates an arrow to match its current velocity vector.
   /// \param arrow arrow projectile whose rotation is updated.
   void updateRotation(Arrow* arrow);
   Arrow* _loaded_arrow = nullptr;
   b2Body* _launcher_body = nullptr;

   static WeaponProperties _default_properties;
};
