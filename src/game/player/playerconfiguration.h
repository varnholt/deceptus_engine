#ifndef PLAYERCONFIGURATION_H
#define PLAYERCONFIGURATION_H

#include <string>

struct PlayerConfiguration
{
   PlayerConfiguration() = default;

   float mSpeedMaxWalk = 2.5f;
   float mSpeedMaxRun = 4.5f;
   float mSpeedMaxWater = 2.0f;
   float mPlayerFriction = 0.0f;
   float mPlayerJumpStrength = 6.2f;
   float mAccelerationGround = 0.25f;
   float mAccelerationAir = 0.15f;
   float mSlowdownGround = 0.6f;
   float mSlowdownAir = 0.65f;
   int mJumpSteps = 6;
   float mJumpFalloff = 6.0f;
   float mJumpSpeedFactor = 0.1f;

   std::string serialize();
   void deserialize(const std::string& data);
   void serializeToFile(const std::string& data, const std::string& filename);

   static PlayerConfiguration sInstance;

   static PlayerConfiguration& getInstance()
   {
      return sInstance;
   }
};

#endif // PLAYERCONFIGURATION_H
