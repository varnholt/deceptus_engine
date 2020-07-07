#include "playerjump.h"

#include "animationpool.h"
#include "audio.h"
#include "globalclock.h"
#include "physicsconfiguration.h"

#include <Box2D/Box2D.h>
#include <iostream>


//----------------------------------------------------------------------------------------------------------------------
void PlayerJump::update(b2Body* body, bool inAir, bool inWater, bool crouching, bool climbing, const PlayerControls& controls)
{
#ifdef JUMP_GRAVITY_SCALING
   if (mInAir && !inAir)
   {
      // std::cout << "reset" << std::endl;
      body->SetGravityScale(1.0f);
   }

   if (mInWater)
   {
      body->SetGravityScale(0.5f);
   }
#endif

   mInAir = inAir;
   mInWater = inWater;

   mCrouching = crouching;
   mClimbing = climbing;

   mJumpButtonPressed = controls.isJumpButtonPressed();

   updateLostGroundContact();
   updateJump(body);
   updateJumpBuffer();
}


//----------------------------------------------------------------------------------------------------------------------
void PlayerJump::updateJumpBuffer()
{
   if (mInAir)
   {
      return;
   }

   // if jump is pressed while the ground is just a few centimeters away,
   // store the information and jump as soon as the places touches ground
   auto now = GlobalClock::getInstance()->getElapsedTime();
   auto timeDiff = (now - mLastJumpPressTime).asMilliseconds();

   if (timeDiff < PhysicsConfiguration::getInstance().mPlayerJumpBufferMs)
   {
      jump();
   }
}


//----------------------------------------------------------------------------------------------------------------------
void PlayerJump::updateJump(b2Body* body)
{
   if (mInWater && mJumpButtonPressed)
   {
      body->ApplyForce(b2Vec2(0, -1.0f), body->GetWorldCenter(), true);
   }
   else if (
         (mJumpSteps > 0 && mJumpButtonPressed)
      || mJumpClock.getElapsedTime().asMilliseconds() < PhysicsConfiguration::getInstance().mPlayerJumpMinimalDurationMs
   )
   {
      // jump higher if a faster
      auto maxWalk = PhysicsConfiguration::getInstance().mPlayerSpeedMaxWalk;
      auto vel = fabs(body->GetLinearVelocity().x) - maxWalk;
      auto factor = 1.0f;

      if (vel > 0.0f)
      {
         auto maxRun = PhysicsConfiguration::getInstance().mPlayerSpeedMaxRun;

         factor =
              1.0f
            + PhysicsConfiguration::getInstance().mPlayerJumpSpeedFactor
            * (vel / (maxRun - maxWalk));
      }

      /*
       * +---+
         |###|
         |###| <- current speed => factor
         |###|
         +###+
         |   |
         |   |
         |   |
       * +---+
      */

     // to change velocity by 5 in one time step
     auto force = factor * body->GetMass() * PhysicsConfiguration::getInstance().mPlayerJumpStrength / (1.0f / 60.0f) /*dt*/; //f = mv/t

     // spread this over 6 time steps
     force /= PhysicsConfiguration::getInstance().mPlayerJumpFalloff;

     // printf("force: %f\n", force);
     body->ApplyForceToCenter(b2Vec2(0.0f, -force), true );

     mJumpSteps--;

     if (mJumpSteps == 0)
     {
        body->SetGravityScale(1.35);
        // std::cout << "blam" << std::endl;
     }
   }
   else
   {
      mJumpSteps = 0;
   }
}


//----------------------------------------------------------------------------------------------------------------------
void PlayerJump::jumpImpulse(b2Body* body)
{
   mJumpClock.restart();

   float impulse = body->GetMass() * 6.0f;

   body->ApplyLinearImpulse(
      b2Vec2(0.0f, -impulse),
      body->GetWorldCenter(),
      true
   );
}


//----------------------------------------------------------------------------------------------------------------------
void PlayerJump::jumpForce()
{
   mJumpClock.restart();
   mJumpSteps = PhysicsConfiguration::getInstance().mPlayerJumpSteps;
}


//----------------------------------------------------------------------------------------------------------------------
void PlayerJump::jump()
{
   if (mCrouching)
   {
      return;
   }

   sf::Time elapsed = mJumpClock.getElapsedTime();

   // only allow a new jump after a a couple of milliseconds
   if (elapsed.asMilliseconds() > 100)
   {
      if (!mInAir || mGroundContactJustLost || mClimbing)
      {
         mRemoveClimbJoint();
         mClimbing = false;

         jumpForce();

         if (mInWater)
         {
            // play some waterish sample?
         }
         else
         {
            mDustAnimation();
            Audio::getInstance()->playSample("jump.wav");
         }
      }
      else
      {
         // player pressed jump but is still in air.
         // buffer that information to trigger the jump a few millis later.
         if (mInAir)
         {
            mLastJumpPressTime = GlobalClock::getInstance()->getElapsedTime();
         }
      }
   }
}



//----------------------------------------------------------------------------------------------------------------------
void PlayerJump::updateLostGroundContact()
{
   // when losing contact to the ground allow jumping for 2-3 more frames
   //
   // if player had ground contact in previous frame but now lost ground
   // contact then start counting to 200ms
   if (mHadGroundContact && mInAir && !isJumping())
   {
      auto now = GlobalClock::getInstance()->getElapsedTime();
      mGroundContactLostTime = now;
      mGroundContactJustLost = true;
   }

   // flying now, probably allow jump
   else if (mInAir)
   {
      auto now = GlobalClock::getInstance()->getElapsedTime();
      auto timeDiff = (now - mGroundContactLostTime).asMilliseconds();
      mGroundContactJustLost = (timeDiff < PhysicsConfiguration::getInstance().mPlayerJumpAfterContactLostMs);

      // if (mGroundContactJustLost)
      // {
      //    std::cout << "allowed to jump for another " << timeDiff << "ms" << std::endl;
      // }
   }
   else
   {
      mGroundContactJustLost = false;
   }

   mHadGroundContact = !mInAir;
}


//----------------------------------------------------------------------------------------------------------------------
bool PlayerJump::isJumping() const
{
   return (mJumpSteps > 0);
}
