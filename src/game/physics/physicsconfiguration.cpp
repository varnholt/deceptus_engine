#include "physicsconfiguration.h"

#include <fstream>
#include <iostream>
#include <iomanip>
#include <ostream>
#include <sstream>

#include "json/json.hpp"

using json = nlohmann::json;


bool PhysicsConfiguration::__initialized = false;


std::string PhysicsConfiguration::serialize()
{
   // create a JSON value with different types
   json config =
   {
      {
         "PhysicsConfiguration",
         {
            {"timestep",                                          _time_step},
            {"gravity",                                           _gravity},

            {"player_speed_max_air",                              _player_speed_max_air},
            {"player_speed_max_walk",                             _player_speed_max_walk},
            {"player_speed_max_run",                              _player_speed_max_run},
            {"player_speed_max_water",                            _player_speed_max_water},
            {"player_friction",                                   _player_friction},
            {"player_acceleration_ground",                        _player_acceleration_ground},
            {"player_deceleration_ground",                        _player_deceleration_ground},
            {"player_acceleration_air",                           _player_acceleration_air},
            {"player_deceleration_air",                           _player_deceleration_air},

            {"player_jump_strength",                              _player_jump_strength},
            {"player_jump_steps",                                 _player_jump_frame_count},
            {"player_jump_after_contact_lost_in_ms",              _player_jump_after_contact_lost_ms},
            {"player_jump_buffer_in_ms",                          _player_jump_buffer_ms},
            {"player_jump_minimal_duration_in_ms",                _player_jump_minimal_duration_ms},
            {"player_jump_falloff",                               _player_jump_falloff},
            {"player_jump_speed_factor",                          _player_jump_speed_factor},

            {"player_dash_frame_count",                           _player_dash_frame_count},
            {"player_dash_multiplier",                            _player_dash_multiplier},
            {"player_dash_multiplier_increment_per_frame",        _player_dash_multiplier_increment_per_frame},
            {"player_dash_multiplier_scale_per_frame",            _player_dash_multiplier_scale_per_frame},
            {"player_dash_vector",                                _player_dash_vector},

            {"player_wall_slide_friction",                        _player_wall_slide_friction},

            {"player_wall_jump_frame_count",                      _player_wall_jump_frame_count},
            {"player_wall_jump_vector_x",                         _player_wall_jump_vector_y},
            {"player_wall_jump_vector_y",                         _player_wall_jump_vector_x},
            {"player_wall_jump_multiplier",                       _player_wall_jump_multiplier},
            {"player_wall_jump_multiplier_increment_per_frame",   _player_wall_jump_multiplier_increment_per_frame},
            {"player_wall_jump_multiplier_scale_per_frame",       _player_wall_jump_multiplier_scale_per_frame},

            {"player_double_jump_factor",                         _player_double_jump_factor},

            {"player_hard_landing_damage_enabled",                _player_hard_landing_damage_enabled},
            {"player_hard_landing_damage_factor",                 _player_hard_landing_damage_factor},
         }
      }
   };

   std::stringstream sstream;
   sstream << std::setw(4) << config << "\n\n";
   return sstream.str();
}


void PhysicsConfiguration::deserialize(const std::string& data)
{
   json config;
   try {
      config = json::parse(data);
   }
   catch (const std::exception& e)
   {
      std::cerr << e.what() << std::endl;
   }

   _time_step                                       = config["PhysicsConfiguration"]["timestep"].get<float>();
   _gravity                                         = config["PhysicsConfiguration"]["gravity"].get<float>();

   _player_speed_max_walk                           = config["PhysicsConfiguration"]["player_speed_max_walk"].get<float>();
   _player_speed_max_run                            = config["PhysicsConfiguration"]["player_speed_max_run"].get<float>();
   _player_speed_max_water                          = config["PhysicsConfiguration"]["player_speed_max_water"].get<float>();
   _player_speed_max_air                            = config["PhysicsConfiguration"]["player_speed_max_air"].get<float>();
   _player_friction                                 = config["PhysicsConfiguration"]["player_friction"].get<float>();

   _player_acceleration_ground                      = config["PhysicsConfiguration"]["player_acceleration_ground"].get<float>();
   _player_acceleration_air                         = config["PhysicsConfiguration"]["player_acceleration_air"].get<float>();
   _player_deceleration_ground                      = config["PhysicsConfiguration"]["player_deceleration_ground"].get<float>();
   _player_deceleration_air                         = config["PhysicsConfiguration"]["player_deceleration_air"].get<float>();

   _player_jump_strength                            = config["PhysicsConfiguration"]["player_jump_strength"].get<float>();
   _player_jump_frame_count                         = config["PhysicsConfiguration"]["player_jump_steps"].get<int32_t>();
   _player_jump_after_contact_lost_ms               = config["PhysicsConfiguration"]["player_jump_after_contact_lost_in_ms"].get<int32_t>();
   _player_jump_buffer_ms                           = config["PhysicsConfiguration"]["player_jump_buffer_in_ms"].get<int32_t>();
   _player_jump_minimal_duration_ms                 = config["PhysicsConfiguration"]["player_jump_minimal_duration_in_ms"].get<int32_t>();
   _player_jump_falloff                             = config["PhysicsConfiguration"]["player_jump_falloff"].get<float>();
   _player_jump_speed_factor                        = config["PhysicsConfiguration"]["player_jump_speed_factor"].get<float>();

   _player_dash_frame_count                         = config["PhysicsConfiguration"]["player_dash_frame_count"].get<int32_t>();
   _player_dash_multiplier                          = config["PhysicsConfiguration"]["player_dash_multiplier"].get<float>();
   _player_dash_multiplier_increment_per_frame      = config["PhysicsConfiguration"]["player_dash_multiplier_increment_per_frame"].get<float>();
   _player_dash_multiplier_scale_per_frame          = config["PhysicsConfiguration"]["player_dash_multiplier_scale_per_frame"].get<float>();
   _player_dash_vector                              = config["PhysicsConfiguration"]["player_dash_vector"].get<float>();

   _player_wall_slide_friction                      = config["PhysicsConfiguration"]["player_wall_slide_friction"].get<float>();
   _player_wall_jump_frame_count                    = config["PhysicsConfiguration"]["player_wall_jump_frame_count"].get<int32_t>();
   _player_wall_jump_vector_x                       = config["PhysicsConfiguration"]["player_wall_jump_vector_x"].get<float>();
   _player_wall_jump_vector_y                       = config["PhysicsConfiguration"]["player_wall_jump_vector_y"].get<float>();
   _player_wall_jump_multiplier                     = config["PhysicsConfiguration"]["player_wall_jump_multiplier"].get<float>();
   _player_wall_jump_multiplier_increment_per_frame = config["PhysicsConfiguration"]["player_wall_jump_multiplier_increment_per_frame"].get<float>();
   _player_wall_jump_multiplier_scale_per_frame     = config["PhysicsConfiguration"]["player_wall_jump_multiplier_scale_per_frame"].get<float>();

   _player_double_jump_factor                       = config["PhysicsConfiguration"]["player_double_jump_factor"].get<float>();

   _player_hard_landing_damage_enabled              = config["PhysicsConfiguration"]["player_hard_landing_damage_enabled"].get<bool>();
   _player_hard_landing_damage_factor               = config["PhysicsConfiguration"]["player_hard_landing_damage_factor"].get<float>();
}


void PhysicsConfiguration::deserializeFromFile(const std::string &filename)
{
  std::ifstream ifs (filename, std::ifstream::in);

  char c = ifs.get();
  std::string data;

  while (ifs.good())
  {
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


PhysicsConfiguration& PhysicsConfiguration::getInstance()
{
   static PhysicsConfiguration __instance;

   if (!__initialized)
   {
      __initialized = true;
      __instance.deserializeFromFile();
   }

   return __instance;
}

