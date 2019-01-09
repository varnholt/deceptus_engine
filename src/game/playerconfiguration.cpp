#include "playerconfiguration.h"

#include <fstream>
#include <iostream>
#include <ostream>
#include <sstream>

#include "json/json.hpp"
using json = nlohmann::json;


std::string PlayerConfiguration::serialize()
{
   // create a JSON value with different types
   json config =
   {
      "playerconfiguration",
      {
         {"speed_max_walk",       mSpeedMaxWalk},
         {"speed_max_run",        mSpeedMaxRun},
         {"speed_max_water",      mSpeedMaxWater},
         {"player_friction",      mPlayerFriction},
         {"player_jump_strength", mPlayerJumpStrength},
         {"acceleration_ground",  mAccelerationGround},
         {"acceleration_air",     mAccelerationAir},
         {"slowdown_ground",      mSlowdownGround},
         {"slowdown_air",         mSlowdownAir},
         {"jump_steps",           mJumpSteps},
         {"jump_falloff",         mJumpFalloff},
         {"jump_speedFactor",     mJumpSpeedFactor}
      }
   };

   std::stringstream sstream;
   sstream << std::setw(4) << config << "\n\n";
   return sstream.str();
}


void PlayerConfiguration::deserialize(const std::string& data)
{
   json config = json::parse(data);
   mSpeedMaxWalk       = config["playerconfiguration"]["speed_max_walk"].get<float>();
   mSpeedMaxRun        = config["playerconfiguration"]["speed_max_run"].get<float>();
   mSpeedMaxWater      = config["playerconfiguration"]["speed_max_water"].get<float>();
   mPlayerFriction     = config["playerconfiguration"]["player_friction"].get<float>();
   mPlayerJumpStrength = config["playerconfiguration"]["player_jump_strength"].get<float>();
   mAccelerationGround = config["playerconfiguration"]["acceleration_ground"].get<float>();
   mAccelerationAir    = config["playerconfiguration"]["acceleration_air"].get<float>();
   mSlowdownGround     = config["playerconfiguration"]["slowdown_ground"].get<float>();
   mSlowdownAir        = config["playerconfiguration"]["slowdown_air"].get<float>();
   mJumpSteps          = config["playerconfiguration"]["jump_steps"].get<int>();
   mJumpFalloff        = config["playerconfiguration"]["jump_falloff"].get<float>();
   mJumpSpeedFactor    = config["playerconfiguration"]["jump_speedFactor"].get<float>();
}


void PlayerConfiguration::serializeToFile(const std::string &data, const std::string &filename)
{
   std::ofstream file(filename);
   file << data;
}

/*

  std::ifstream ifs ("test.txt", std::ifstream::in);

  char c = ifs.get();

  while (ifs.good()) {
    std::cout << c;
    c = ifs.get();
  }

  ifs.close();

*/
