#pragma once

#include <string>

struct PhysicsConfiguration
{
   PhysicsConfiguration() = default;

   float mTimeStep = 1.0f/60.0f;
   float mGravity = 8.5f;

   float mPlayerSpeedMaxWalk = 2.5f;
   float mPlayerSpeedMaxRun = 3.5f;
   float mPlayerSpeedMaxWater = 1.5f;
   float mPlayerFriction = 0.0f;
   float mPlayerJumpStrength = 3.3f;
   float mPlayerAccelerationGround = 0.1f;
   float mPlayerAccelerationAir = 0.05f;
   float mPlayerDecelerationGround = 0.6f;
   float mPlayerDecelerationAir = 0.65f;
   int mPlayerJumpSteps = 9;
   int mPlayerJumpAfterContactLostMs = 100;
   int mPlayerJumpBufferMs = 100;
   float mPlayerJumpFalloff = 6.5f;
   float mPlayerJumpSpeedFactor = 0.1f;

   std::string serialize();
   void deserialize(const std::string& data);

   void deserializeFromFile(const std::string& filename = "data/config/physics.json");
   void serializeToFile(const std::string& filename = "data/config/physics.json");

   static PhysicsConfiguration sInstance;

   static PhysicsConfiguration& getInstance()
   {
      return sInstance;
   }
};

