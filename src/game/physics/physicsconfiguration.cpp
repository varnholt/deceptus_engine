#include "physicsconfiguration.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <sstream>

#include "json/json.hpp"

using json = nlohmann::json;

std::string PhysicsConfiguration::serialize()
{
   // create a JSON value with different types
   json config = {
      {"PhysicsConfiguration",
       {
          {"timestep", _time_step},
          {"gravity", _gravity},

          {"player_speed_max_air", _player_speed_max_air},
          {"player_speed_max_walk", _player_speed_max_walk},
          {"player_speed_max_run", _player_speed_max_run},
          {"player_speed_max_water", _player_speed_max_water},
          {"player_friction", _player_friction},

          {"player_acceleration_ground", _player_acceleration_ground},
          {"player_deceleration_ground", _player_deceleration_ground},
          {"player_acceleration_air", _player_acceleration_air},
          {"player_deceleration_air", _player_deceleration_air},
          {"player_acceleration_water", _player_acceleration_water},
          {"player_deceleration_sword_attack", _player_deceleration_sword_attack},

          {"player_max_velocity_horizontal", _player_max_velocity_horizontal},
          {"player_max_velocity_up", _player_max_velocity_up},
          {"player_max_velocity_down", _player_max_velocity_down},

          {"player_jump_strength", _player_jump_strength},
          {"player_jump_steps", _player_jump_frame_count},
          {"player_jump_steps_minimum", _player_jump_frame_count_minimum},
          {"player_jump_after_contact_lost_in_ms", _player_jump_after_contact_lost_ms},
          {"player_jump_buffer_in_ms", _player_jump_buffer_ms},
          {"player_jump_minimal_duration_in_ms", _player_jump_minimal_duration_ms},
          {"player_jump_falloff", _player_jump_falloff},
          {"player_jump_speed_factor", _player_jump_speed_factor},

          {"player_dash_frame_count", _player_dash_frame_count},
          {"player_dash_multiplier", _player_dash_multiplier},
          {"player_dash_multiplier_increment_per_frame", _player_dash_multiplier_increment_per_frame},
          {"player_dash_multiplier_scale_per_frame", _player_dash_multiplier_scale_per_frame},
          {"player_dash_vector", _player_dash_vector},

          {"player_wall_slide_friction", _player_wall_slide_friction},

          {"player_wall_jump_frame_count", _player_wall_jump_frame_count},
          {"player_wall_jump_vector_x", _player_wall_jump_vector_y},
          {"player_wall_jump_vector_y", _player_wall_jump_vector_x},
          {"player_wall_jump_multiplier", _player_wall_jump_multiplier},
          {"player_wall_jump_multiplier_increment_per_frame", _player_wall_jump_multiplier_increment_per_frame},
          {"player_wall_jump_multiplier_scale_per_frame", _player_wall_jump_multiplier_scale_per_frame},
          {"player_wall_jump_extra_force", _player_wall_jump_extra_force},
          {"player_wall_jump_lock_key_duration_ms", _player_wall_jump_lock_key_duration_ms},

          {"player_double_jump_factor", _player_double_jump_factor},

          {"player_hard_landing_damage_enabled", _player_hard_landing_damage_enabled},
          {"player_hard_landing_damage_factor", _player_hard_landing_damage_factor},
          {"player_hard_landing_delay_s", _player_hard_landing_delay_s},

          {"player_in_water_force_jump_button", _player_in_water_force_jump_button},
          {"player_in_water_time_to_allow_jump_button_ms", _player_in_water_time_to_allow_jump_button_ms},
          {"player_in_water_linear_velocity_y_clamp_min", _player_in_water_linear_velocity_y_clamp_min},
          {"player_in_water_linear_velocity_y_clamp_max", _player_in_water_linear_velocity_y_clamp_max},
          {"in_water_buoyancy_force", _in_water_buoyancy_force},
       }}
   };

   std::stringstream sstream;
   sstream << std::setw(4) << config << "\n\n";
   return sstream.str();
}

void PhysicsConfiguration::deserialize(const std::string& data)
{
   PhysicsConfiguration reset_values;
   *this = reset_values;

   json config;
   try
   {
      config = json::parse(data);
   }
   catch (const std::exception& e)
   {
      std::cerr << e.what() << std::endl;
      return;
   }

   const auto& physics_config = config["PhysicsConfiguration"];

   auto get_value_if_exists = [](const json& j, const std::string& key, auto& var)
   {
      if ((j.find(key) != j.end()) && !j[key].is_null())
      {
         var = j[key].get<std::decay_t<decltype(var)>>();
      }
   };

   get_value_if_exists(physics_config, "timestep", _time_step);
   get_value_if_exists(physics_config, "gravity", _gravity);

   get_value_if_exists(physics_config, "player_speed_max_walk", _player_speed_max_walk);
   get_value_if_exists(physics_config, "player_speed_max_run", _player_speed_max_run);
   get_value_if_exists(physics_config, "player_speed_max_water", _player_speed_max_water);
   get_value_if_exists(physics_config, "player_speed_max_air", _player_speed_max_air);
   get_value_if_exists(physics_config, "player_friction", _player_friction);

   get_value_if_exists(physics_config, "player_acceleration_ground", _player_acceleration_ground);
   get_value_if_exists(physics_config, "player_acceleration_air", _player_acceleration_air);

   get_value_if_exists(physics_config, "player_deceleration_ground", _player_deceleration_ground);
   get_value_if_exists(physics_config, "player_deceleration_air", _player_deceleration_air);
   get_value_if_exists(physics_config, "player_deceleration_sword_attack", _player_deceleration_sword_attack);

   get_value_if_exists(physics_config, "player_max_velocity_horizontal", _player_max_velocity_horizontal);
   get_value_if_exists(physics_config, "player_max_velocity_up", _player_max_velocity_up);
   get_value_if_exists(physics_config, "player_max_velocity_down", _player_max_velocity_down);

   get_value_if_exists(physics_config, "player_jump_strength", _player_jump_strength);
   get_value_if_exists(physics_config, "player_jump_steps", _player_jump_frame_count);
   get_value_if_exists(physics_config, "player_jump_steps_minimum", _player_jump_frame_count_minimum);

   get_value_if_exists(physics_config, "player_jump_after_contact_lost_in_ms", _player_jump_after_contact_lost_ms);
   get_value_if_exists(physics_config, "player_jump_buffer_in_ms", _player_jump_buffer_ms);
   get_value_if_exists(physics_config, "player_jump_minimal_duration_in_ms", _player_jump_minimal_duration_ms);
   get_value_if_exists(physics_config, "player_jump_falloff", _player_jump_falloff);
   get_value_if_exists(physics_config, "player_jump_speed_factor", _player_jump_speed_factor);

   get_value_if_exists(physics_config, "player_dash_frame_count", _player_dash_frame_count);
   get_value_if_exists(physics_config, "player_dash_multiplier", _player_dash_multiplier);
   get_value_if_exists(physics_config, "player_dash_multiplier_increment_per_frame", _player_dash_multiplier_increment_per_frame);
   get_value_if_exists(physics_config, "player_dash_multiplier_scale_per_frame", _player_dash_multiplier_scale_per_frame);
   get_value_if_exists(physics_config, "player_dash_vector", _player_dash_vector);

   get_value_if_exists(physics_config, "player_wall_slide_friction", _player_wall_slide_friction);
   get_value_if_exists(physics_config, "player_wall_jump_frame_count", _player_wall_jump_frame_count);
   get_value_if_exists(physics_config, "player_wall_jump_vector_x", _player_wall_jump_vector_x);
   get_value_if_exists(physics_config, "player_wall_jump_vector_y", _player_wall_jump_vector_y);
   get_value_if_exists(physics_config, "player_wall_jump_multiplier", _player_wall_jump_multiplier);
   get_value_if_exists(physics_config, "player_wall_jump_multiplier_increment_per_frame", _player_wall_jump_multiplier_increment_per_frame);
   get_value_if_exists(physics_config, "player_wall_jump_multiplier_scale_per_frame", _player_wall_jump_multiplier_scale_per_frame);
   get_value_if_exists(physics_config, "player_wall_jump_extra_force", _player_wall_jump_extra_force);
   get_value_if_exists(physics_config, "player_wall_jump_lock_key_duration_ms", _player_wall_jump_lock_key_duration_ms);

   get_value_if_exists(physics_config, "player_double_jump_factor", _player_double_jump_factor);

   get_value_if_exists(physics_config, "player_hard_landing_damage_enabled", _player_hard_landing_damage_enabled);
   get_value_if_exists(physics_config, "player_hard_landing_damage_factor", _player_hard_landing_damage_factor);
   get_value_if_exists(physics_config, "player_hard_landing_delay_s", _player_hard_landing_delay_s);

   get_value_if_exists(physics_config, "player_in_water_force_jump_button", _player_in_water_force_jump_button);
   get_value_if_exists(physics_config, "player_in_water_time_to_allow_jump_button_ms", _player_in_water_time_to_allow_jump_button_ms);
   get_value_if_exists(physics_config, "player_in_water_linear_velocity_y_clamp_min", _player_in_water_linear_velocity_y_clamp_min);
   get_value_if_exists(physics_config, "player_in_water_linear_velocity_y_clamp_max", _player_in_water_linear_velocity_y_clamp_max);
   get_value_if_exists(physics_config, "in_water_buoyancy_force", _in_water_buoyancy_force);
}

void PhysicsConfiguration::deserializeFromFile(const std::string& filename)
{
   std::ifstream ifs(filename, std::ifstream::in);

   char c = static_cast<char>(ifs.get());
   std::string data;

   while (ifs.good())
   {
      data.push_back(c);
      c = static_cast<char>(ifs.get());
   }

   ifs.close();

   deserialize(data);
}

void PhysicsConfiguration::serializeToFile(const std::string& filename)
{
   std::string data = serialize();
   std::ofstream file(filename);
   file << data;
}

PhysicsConfiguration& PhysicsConfiguration::getInstance()
{
   static bool __initialized = false;
   static PhysicsConfiguration __instance;

   if (!__initialized)
   {
      __initialized = true;
      __instance.deserializeFromFile();
   }

   return __instance;
}
