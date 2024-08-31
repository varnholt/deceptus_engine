#include "playerplatform.h"

#include "game/physics/gamecontactlistener.h"

void PlayerPlatform::update(b2Body* body, bool jumping)
{
   if (jumping)
   {
      if (_platform_gravity_scale.has_value())
      {
         body->SetGravityScale(_platform_gravity_scale.value());
         _platform_gravity_scale.reset();
      }

      return;
   }

   if (isOnPlatform() && _platform_body)
   {
      if (!_platform_gravity_scale.has_value())
      {
         _platform_gravity_scale = body->GetGravityScale();
      }

      const auto x = body->GetPosition().x + _platform_dx;
      const auto y = body->GetPosition().y;
      body->SetTransform(b2Vec2(x, y), 0.0f);
      body->SetGravityScale(_gravity_scale);

      // printf("standing on platform, x: %f, y: %f, dx: %f \n", x, y, dx);
   }
   else if (_platform_gravity_scale.has_value())
   {
      body->SetGravityScale(_platform_gravity_scale.value());
      _platform_gravity_scale.reset();
   }
}

void PlayerPlatform::reset()
{
   // reset bodies passed from the contact listener
   _platform_body = nullptr;

   _platform_gravity_scale.reset();
}

void PlayerPlatform::setGravityScale(float scale)
{
   _gravity_scale = scale;
}

void PlayerPlatform::setPlatformBody(b2Body* body)
{
   _platform_body = body;
}

b2Body* PlayerPlatform::getPlatformBody() const
{
   return _platform_body;
}

void PlayerPlatform::setPlatformDx(float dx)
{
   _platform_dx = dx;
}

bool PlayerPlatform::isOnPlatform() const
{
   const auto contact_listener = GameContactListener::getInstance();
   const auto has_moving_platform_contacts = contact_listener.getMovingPlatformContactCount() > 0;
   const auto has_death_block_contacts = contact_listener.getDeathBlockContactCount() > 0;
   const auto on_platform = (has_moving_platform_contacts || has_death_block_contacts) && isOnGround();
   return on_platform;
}

bool PlayerPlatform::isOnGround() const
{
   return GameContactListener::getInstance().getPlayerFootContactCount() > 0;
}
