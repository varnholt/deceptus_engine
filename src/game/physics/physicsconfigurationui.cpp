#include "physicsconfigurationui.h"
#include "physicsconfiguration.h"

#pragma warning(push, 0)
#include "imgui/imgui-SFML.h"
#include "imgui/imgui.h"
#pragma warning(pop)

#include <iostream>
#include <sstream>

PhysicsConfigurationUi::PhysicsConfigurationUi()
    : _render_window(std::make_unique<sf::RenderWindow>(sf::VideoMode(800, 800), "deceptus physics configuration"))
{
   if (!ImGui::SFML::Init(*_render_window.get()))
   {
      std::cout << "could not create render window" << std::endl;
   }
}

void PhysicsConfigurationUi::processEvents()
{
   sf::Event event;
   while (_render_window->pollEvent(event))
   {
      ImGui::SFML::ProcessEvent(*_render_window.get(), event);

      if (event.type == sf::Event::Closed)
      {
         _render_window->close();
      }
   }
}

void PhysicsConfigurationUi::draw()
{
   auto drawFloatElement = [](const std::string& name, auto* value, auto min, auto max)
   {
      std::stringstream stream_input;
      std::stringstream stream_slider;
      stream_input << "##" << name << "_input";
      stream_slider << "##" << name << "_slider";

      ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.2f);

      if (ImGui::InputFloat(stream_input.str().c_str(), value, 0.1f, 1.0f, "%.3f"))
      {
         *value = std::clamp(*value, min, max);
      }

      ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.35f);
      ImGui::SameLine();
      ImGui::SliderFloat(stream_slider.str().c_str(), value, min, max);
      ImGui::SameLine();
      ImGui::Text("%s", name.c_str());
   };

   auto drawIntElement = [](const std::string& name, auto* value, auto min, auto max)
   {
      std::stringstream stream_input;
      std::stringstream stream_slider;
      stream_input << "##" << name << "input";
      stream_slider << "##" << name << "slider";

      ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.2f);

      if (ImGui::InputInt(stream_input.str().c_str(), value))
      {
         *value = std::clamp(*value, min, max);
      }

      ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.35f);
      ImGui::SameLine();
      ImGui::SliderInt(stream_slider.str().c_str(), value, min, max);
      ImGui::SameLine();
      ImGui::Text("%s", name.c_str());
   };

   auto& config = PhysicsConfiguration::getInstance();

   ImGui::SFML::Update(*_render_window.get(), _clock.restart());

   ImGui::Begin("physics");
   ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.50f);
   ImGuiTreeNodeFlags header_flags = ImGuiTreeNodeFlags_DefaultOpen;

   if (ImGui::BeginMainMenuBar())
   {
      if (ImGui::BeginMenu("File", true))
      {
         if (ImGui::MenuItem("Save Physics"))
         {
            std::cout << "save physics" << std::endl;
            config.serializeToFile();
         }

         if (ImGui::MenuItem("Reload Physics"))
         {
            std::cout << "reload physics" << std::endl;
            config.deserializeFromFile();
         }

         ImGui::EndMenu();
      }
      ImGui::EndMainMenuBar();
   }

   if (ImGui::CollapsingHeader("gravity", header_flags))
   {
      drawFloatElement("global gravity (requires level reload)", &config._gravity, -50.0f, 50.0f);
      drawFloatElement("gravity scale default", &config._gravity_scale_default, 0.1f, 3.0f);
      drawFloatElement("gravity scale water", &config._gravity_scale_water, 0.1f, 1.0f);
      drawFloatElement("gravity scale jump downward", &config._gravity_scale_jump_downward, 0.5f, 3.0f);
   }

   if (ImGui::CollapsingHeader("player velocity", header_flags))
   {
      drawFloatElement("speed max walk", &config._player_speed_max_walk, 0.1f, 20.0f);
      drawFloatElement("speed max run", &config._player_speed_max_run, 0.1f, 20.0f);
      drawFloatElement("speed max water", &config._player_speed_max_water, 0.1f, 20.0f);
      drawFloatElement("speed max air", &config._player_speed_max_air, 0.1f, 20.0f);
      drawFloatElement("friction", &config._player_friction, 0.0f, 1.0f);
      drawFloatElement("jump strength", &config._player_jump_strength, 0.1f, 20.0f);
      drawFloatElement("acceleration ground", &config._player_acceleration_ground, 0.01f, 2.0f);
      drawFloatElement("acceleration air", &config._player_acceleration_air, 0.01f, 2.0f);
      drawFloatElement("deceleration ground", &config._player_deceleration_ground, 0.01f, 2.0f);
      drawFloatElement("deceleration air", &config._player_deceleration_air, 0.01f, 2.0f);
      drawFloatElement("cap velocity horizontal", &config._player_max_velocity_horizontal, 0.1f, 30.0f);
      drawFloatElement("cap velocity up", &config._player_max_velocity_up, 0.1f, 30.0f);
      drawFloatElement("cap velocity down", &config._player_max_velocity_down, 0.1f, 30.0f);
   }

   if (ImGui::CollapsingHeader("player jump", header_flags))
   {
      const auto jump_frame_count_min = 1;
      const auto jump_frame_count_max = 50;
      const auto jump_frame_count_min_min = 0;
      auto jump_frame_count_min_max = config._player_jump_frame_count - 1;

      if (jump_frame_count_min_max < jump_frame_count_min_min)
      {
         jump_frame_count_min_max = jump_frame_count_min_min;
      }

      drawIntElement("jump frame count", &config._player_jump_frame_count, jump_frame_count_min, jump_frame_count_max);
      drawIntElement(
         "jump frame count minimum", &config._player_jump_frame_count_minimum, jump_frame_count_min_min, jump_frame_count_min_max
      );
      drawIntElement("jump after contact lost [ms]", &config._player_jump_after_contact_lost_ms, jump_frame_count_min_min, 200);
      drawIntElement("jump buffer [ms]", &config._player_jump_buffer_ms, 10, 200);
      drawIntElement("jump minimal duration [ms]", &config._player_jump_minimal_duration_ms, 20, 200);
      drawFloatElement("jump falloff", &config._player_jump_falloff, 1.0f, 20.0f);
      drawFloatElement("jump speed factor", &config._player_jump_speed_factor, 0.0f, 0.5f);
      drawFloatElement("jump impulse factor", &config._player_jump_impulse_factor, 1.0f, 20.0f);
      drawFloatElement("minimum jump interval [ms]", &config._player_minimum_jump_interval_ms, 100.0f, 200.0f);
   }

   if (ImGui::CollapsingHeader("player dash", header_flags))
   {
      drawIntElement("dash frame count", &config._player_dash_frame_count, 1, 100);
      drawFloatElement("dash multiplier", &config._player_dash_multiplier, 1.0f, 100.0f);
      drawFloatElement("dash multiplier inc. per frame", &config._player_dash_multiplier_increment_per_frame, -10.0f, -0.1f);
      drawFloatElement("dash multiplier scale per frame", &config._player_dash_multiplier_scale_per_frame, 0.1f, 5.0f);
      drawFloatElement("dash vector", &config._player_dash_vector, 0.1f, 50.0f);
   }

   if (ImGui::CollapsingHeader("wall slide", header_flags))
   {
      drawFloatElement("wall slide friction", &config._player_wall_slide_friction, 0.0f, 1.0f);
   }

   if (ImGui::CollapsingHeader("wall jump", header_flags))
   {
      drawIntElement("wall jump frame count", &config._player_wall_jump_frame_count, 1, 50);
      drawFloatElement("wall jump vector x", &config._player_wall_jump_vector_x, 0.0f, 20.0f);
      drawFloatElement("wall jump vector y", &config._player_wall_jump_vector_y, 0.0f, 20.0f);
      drawFloatElement("wall jump multiplier", &config._player_wall_jump_multiplier, 1.0f, 50.0f);
      drawFloatElement("wall jump multiplier inc. per frame", &config._player_wall_jump_multiplier_increment_per_frame, -10.0f, -0.1f);
      drawFloatElement("wall jump multiplier scale per frame", &config._player_wall_jump_multiplier_scale_per_frame, 0.1f, 5.0f);
      drawFloatElement("wall jump extra force", &config._player_wall_jump_extra_force, 0.0f, 5.0f);
      drawIntElement("wall jump lock key duration [ms]", &config._player_wall_jump_lock_key_duration_ms, 0, 2000);
   }

   if (ImGui::CollapsingHeader("double jump", header_flags))
   {
      drawFloatElement("double jump factor", &config._player_double_jump_factor, 1.0f, 20.0f);
   }

   if (ImGui::CollapsingHeader("hard landing", header_flags))
   {
      ImGui::Checkbox("hard landing damage enabled", &config._player_hard_landing_damage_enabled);
      drawFloatElement("hard landing damage factor", &config._player_hard_landing_damage_factor, 0.0f, 50.0f);
      drawFloatElement("hard landing delay [s]", &config._player_hard_landing_delay_s, 0.0f, 5.0f);
   }

   if (ImGui::CollapsingHeader("swimming", header_flags))
   {
      drawFloatElement("in water force jump button", &config._player_in_water_force_jump_button, -10.0f, 0.0f);
      drawIntElement("in water time to allow jump button [ms]", &config._player_in_water_time_to_allow_jump_button_ms, 0, 2000);
      drawFloatElement("in water linear velocity y clamp min", &config._player_in_water_linear_velocity_y_clamp_min, -5.0f, 0.0f);
      drawFloatElement("in water linear velocity y clamp max", &config._player_in_water_linear_velocity_y_clamp_max, 0.0f, 5.0f);
      drawFloatElement("in water buoyancy force", &config._in_water_buoyancy_force, 0.0f, 0.2f);
   }

   ImGui::End();

   _render_window->clear();
   ImGui::SFML::Render(*_render_window.get());
   _render_window->display();
}

void PhysicsConfigurationUi::close()
{
   ImGui::SFML::Shutdown();
}
