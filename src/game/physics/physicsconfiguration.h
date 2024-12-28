#pragma once

#include <cstdint>
#include <string>

struct PhysicsConfiguration
{
   PhysicsConfiguration() = default;

   float _time_step = 1.0f / 60.0f;
   float _gravity = 8.5f;

   float _player_speed_max_walk = 2.5f;
   float _player_speed_max_run = 3.5f;
   float _player_speed_max_water = 1.5f;
   float _player_speed_max_air = 4.0f;
   float _player_friction = 0.0f;
   float _player_jump_strength = 3.3f;
   float _player_acceleration_ground = 0.1f;
   float _player_acceleration_air = 0.05f;
   float _player_acceleration_water = 0.02f;
   float _player_deceleration_ground = 0.6f;
   float _player_deceleration_air = 0.65f;
   float _player_deceleration_water = 0.95f;
   float _player_deceleration_sword_attack = 0.95;
   float _player_max_velocity_horizontal = 10.0f;
   float _player_max_velocity_up = 5.0f;
   float _player_max_velocity_down = 10.0f;

   // jump
   int32_t _player_jump_frame_count = 9;
   int32_t _player_jump_frame_count_minimum = 0;
   int32_t _player_jump_after_contact_lost_ms = 100;
   int32_t _player_jump_buffer_ms = 100;
   int32_t _player_jump_minimal_duration_ms = 80;
   float _player_jump_falloff = 6.5f;
   float _player_jump_speed_factor = 0.1f;
   float _player_jump_impulse_factor = 6.0f;      // not in json
   float _player_minimum_jump_interval_ms = 150;  // not in json

   // dash
   int32_t _player_dash_frame_count = 20;
   float _player_dash_multiplier = 20.0f;
   float _player_dash_multiplier_increment_per_frame = -1.0f;
   float _player_dash_multiplier_scale_per_frame = 1.0f;
   float _player_dash_vector = 3.0f;

   // wall slide
   float _player_wall_slide_friction = 0.7f;

   // wall jump
   int32_t _player_wall_jump_frame_count = 20;
   float _player_wall_jump_vector_x = 6.0f;
   float _player_wall_jump_vector_y = 1.0f;
   float _player_wall_jump_multiplier = 20.0f;
   float _player_wall_jump_multiplier_increment_per_frame = -1.0f;
   float _player_wall_jump_multiplier_scale_per_frame = 1.0f;
   float _player_wall_jump_extra_force = 1.75f;
   int32_t _player_wall_jump_lock_key_duration_ms = 500;

   // double jump
   float _player_double_jump_factor = 6.0f;

   // hard landing
   bool _player_hard_landing_damage_enabled = false;
   float _player_hard_landing_damage_factor = 20.0f;
   float _player_hard_landing_delay_s = 1.0f;

   // swimming
   float _player_in_water_force_jump_button = -1.0f;
   int32_t _player_in_water_time_to_allow_jump_button_ms = 500;
   float _player_in_water_linear_velocity_y_clamp_min = -1.5f;
   float _player_in_water_linear_velocity_y_clamp_max = 0.75f;
   float _in_water_buoyancy_force = 0.03f;

   // gravity
   float _gravity_scale_default = 1.0f;         // not in json
   float _gravity_scale_water = 0.5f;           // not in json
   float _gravity_scale_jump_downward = 1.35f;  // not in json

   // sword attack
   int32_t _player_attack_dash_frame_count = 5;
   float _player_attack_dash_multiplier = 5.0f;
   float _player_attack_dash_multiplier_decrement_per_frame = 1.0f;
   float _player_attack_dash_multiplier_scale_per_frame = 1.0f;

   void deserializeFromFile(const std::string& filename = "data/config/physics.json");
   void serializeToFile(const std::string& filename = "data/config/physics.json");

   static PhysicsConfiguration& getInstance();

private:
   std::string serialize();
   void deserialize(const std::string& data);
};
