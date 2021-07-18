#pragma once

#include <cstdint>
#include <string>

struct PhysicsConfiguration
{
   PhysicsConfiguration() = default;

   float mTimeStep = 1.0f/60.0f;
   float mGravity = 8.5f;

   float mPlayerSpeedMaxWalk = 2.5f;
   float mPlayerSpeedMaxRun = 3.5f;
   float mPlayerSpeedMaxWater = 1.5f;
   float mPlayerSpeedMaxAir = 4.0f;
   float mPlayerFriction = 0.0f;
   float mPlayerJumpStrength = 3.3f;
   float mPlayerAccelerationGround = 0.1f;
   float mPlayerAccelerationAir = 0.05f;
   float mPlayerDecelerationGround = 0.6f;
   float mPlayerDecelerationAir = 0.65f;

   // jump
   int32_t mPlayerJumpSteps = 9;
   int32_t mPlayerJumpAfterContactLostMs = 100;
   int32_t mPlayerJumpBufferMs = 100;
   int32_t mPlayerJumpMinimalDurationMs = 80;
   float mPlayerJumpFalloff = 6.5f;
   float mPlayerJumpSpeedFactor = 0.1f;

   // dash
   int32_t mPlayerDashFrameCount = 20;
   float mPlayerDashMultiplier = 20.0f;
   float mPlayerDashMultiplierIncrementPerFrame = -1.0f;
   float mPlayerDashMultiplierScalePerFrame = 1.0f;
   float mPlayerDashVector = 3.0f;

   // wall slide
   float mPlayerWallSlideFriction = 0.4f;

   // wall jump
   int32_t mPlayerWallJumpFrameCount = 20;
   float mPlayerWallJumpVectorX = 6.0f;
   float mPlayerWallJumpVectorY = 1.0f;
   float mPlayerWallJumpMultiplier = 20.0f;
   float mPlayerWallJumpMultiplierIncrementPerFrame = -1.0f;
   float mPlayerWallJumpMultiplierScalePerFrame = 1.0f;

   // double jump
   float mPlayerDoubleJumpFactor = 6.0f;

   void deserializeFromFile(const std::string& filename = "data/config/physics.json");
   void serializeToFile(const std::string& filename = "data/config/physics.json");

   static bool sInitialized;

   static PhysicsConfiguration sInstance;

   static PhysicsConfiguration& getInstance()
   {
      if (!sInitialized)
      {
         sInitialized = true;
         sInstance.deserializeFromFile();
      }

      return sInstance;
   }


private:

   std::string serialize();
   void deserialize(const std::string& data);

};

