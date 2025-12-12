#pragma once

#include "gun.h"

#include "box2d/box2d.h"

struct Arrow;

class Bow : public Gun
{
public:
   Bow(const WeaponProperties& properties = _default_properties);

   void load(b2World* world);
   void use(const std::shared_ptr<b2World>& world, const b2Vec2& pos, const b2Vec2& dir) override;
   void update(const WeaponUpdateData& data) override;
   int32_t getDamage() const override;
   std::string getName() const override;

   b2Body* getLauncherBody() const;
   void setLauncherBody(b2Body* launcher_body);

private:
   void updateRotation(Arrow*);
   Arrow* _loaded_arrow = nullptr;
   b2Body* _launcher_body = nullptr;

   static WeaponProperties _default_properties;
};
