#ifndef PLAYERPLATFORM_H
#define PLAYERPLATFORM_H

#include "box2d/box2d.h"
#include <SFML/Graphics.hpp>
#include <optional>

class PlayerPlatform
{
public:
   PlayerPlatform() = default;

   void update(b2Body* body, bool jumping);
   bool isOnPlatform() const;
   bool isOnGround() const;
   void setPlatformBody(b2Body* body);
   b2Body* getPlatformBody() const;
   void setPlatformDx(float dx_px);
   void setGravityScale(float scale);
   void reset();

private:
   b2Body* _platform_body = nullptr;
   float _platform_dx{0.0f};
   std::optional<float> _platform_gravity_scale;
   float _gravity_scale{10.0f};
};

#endif  // PLAYERPLATFORM_H
