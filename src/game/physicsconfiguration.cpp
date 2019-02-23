#include "physicsconfiguration.h"

#include <fstream>
#include <iostream>
#include <iomanip>
#include <ostream>
#include <sstream>

#include "json/json.hpp"

using json = nlohmann::json;

PhysicsConfiguration PhysicsConfiguration::sInstance;

bool PhysicsConfiguration::sInitialized = false;


std::string PhysicsConfiguration::serialize()
{
   // create a JSON value with different types
   json config =
   {
      {
         "PhysicsConfiguration",
         {
            {"timestep",                             mTimeStep},
            {"gravity",                              mGravity},
            {"player_speed_max_walk",                mPlayerSpeedMaxWalk},
            {"player_speed_max_run",                 mPlayerSpeedMaxRun},
            {"player_speed_max_water",               mPlayerSpeedMaxWater},
            {"player_friction",                      mPlayerFriction},
            {"player_jumps_trength",                 mPlayerJumpStrength},
            {"player_acceleration_ground",           mPlayerAccelerationGround},
            {"player_acceleration_air",              mPlayerAccelerationAir},
            {"player_deceleration_ground",           mPlayerDecelerationGround},
            {"player_deceleration_air",              mPlayerDecelerationAir},
            {"player_jump_steps",                    mPlayerJumpSteps},
            {"player_jump_after_contact_lost_in_ms", mPlayerJumpAfterContactLostMs},
            {"player_jump_buffer_in_ms",             mPlayerJumpBufferMs},
            {"player_jump_falloff",                  mPlayerJumpFalloff},
            {"player_jump_speed_factor",             mPlayerJumpSpeedFactor},
            {"player_dash_steps",                    mPlayerDashSteps},
            {"player_dash_factor",                   mPlayerDashFactor},
         }
      }
   };

   std::stringstream sstream;
   sstream << std::setw(4) << config << "\n\n";
   return sstream.str();
}


void PhysicsConfiguration::deserialize(const std::string& data)
{
   json config = json::parse(data);

   mTimeStep                     = config["PhysicsConfiguration"]["timestep"].get<float>();
   mGravity                      = config["PhysicsConfiguration"]["gravity"].get<float>();
   mPlayerSpeedMaxWalk           = config["PhysicsConfiguration"]["player_speed_max_walk"].get<float>();
   mPlayerSpeedMaxRun            = config["PhysicsConfiguration"]["player_speed_max_run"].get<float>();
   mPlayerSpeedMaxWater          = config["PhysicsConfiguration"]["player_speed_max_water"].get<float>();
   mPlayerFriction               = config["PhysicsConfiguration"]["player_friction"].get<float>();
   mPlayerJumpStrength           = config["PhysicsConfiguration"]["player_jumps_trength"].get<float>();
   mPlayerAccelerationGround     = config["PhysicsConfiguration"]["player_acceleration_ground"].get<float>();
   mPlayerAccelerationAir        = config["PhysicsConfiguration"]["player_acceleration_air"].get<float>();
   mPlayerDecelerationGround     = config["PhysicsConfiguration"]["player_deceleration_ground"].get<float>();
   mPlayerDecelerationAir        = config["PhysicsConfiguration"]["player_deceleration_air"].get<float>();
   mPlayerJumpSteps              = config["PhysicsConfiguration"]["player_jump_steps"].get<int32_t>();
   mPlayerJumpAfterContactLostMs = config["PhysicsConfiguration"]["player_jump_after_contact_lost_in_ms"].get<int32_t>();
   mPlayerJumpBufferMs           = config["PhysicsConfiguration"]["player_jump_buffer_in_ms"].get<int32_t>();
   mPlayerJumpFalloff            = config["PhysicsConfiguration"]["player_jump_falloff"].get<float>();
   mPlayerJumpSpeedFactor        = config["PhysicsConfiguration"]["player_jump_speed_factor"].get<float>();
   mPlayerDashSteps              = config["PhysicsConfiguration"]["player_dash_steps"].get<int32_t>();
   mPlayerDashFactor             = config["PhysicsConfiguration"]["player_dash_factor"].get<float>();
}


void PhysicsConfiguration::deserializeFromFile(const std::string &filename)
{
  std::ifstream ifs (filename, std::ifstream::in);

  char c = ifs.get();
  std::string data;

  while (ifs.good())
  {
    // std::cout << c;
    data.push_back(c);
    c = ifs.get();
  }

  ifs.close();

  deserialize(data);
}


void PhysicsConfiguration::serializeToFile(const std::string &filename)
{
  std::string data = serialize();
  std::ofstream file(filename);
  file << data;
}

