#include "camerasystemconfigurationui.h"
#include "camerasystemconfiguration.h"

#pragma warning(push, 0)
#include "imgui/imgui-SFML.h"
#include "imgui/imgui.h"
#pragma warning(pop)

#include <iostream>
#include <sstream>

CameraSystemConfigurationUi::CameraSystemConfigurationUi()
    : _render_window(std::make_unique<sf::RenderWindow>(sf::VideoMode({800, 400}), "deceptus camera configuration"))
{
   if (!ImGui::SFML::Init(*_render_window.get()))
   {
      std::cout << "could not create render window" << std::endl;
   }
}

void CameraSystemConfigurationUi::processEvents()
{
   while (auto event = _render_window->pollEvent())
   {
      ImGui::SFML::ProcessEvent(*_render_window.get(), event.value());

      if (event->is<sf::Event::Closed>())
      {
         _render_window->close();
      }
   }
}

void CameraSystemConfigurationUi::draw()
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

   auto& config = CameraSystemConfiguration::getInstance();

   ImGui::SFML::Update(*_render_window.get(), _clock.restart());

   ImGui::Begin("camera");
   ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.50f);
   ImGuiTreeNodeFlags header_flags = ImGuiTreeNodeFlags_DefaultOpen;

   if (ImGui::BeginMainMenuBar())
   {
      if (ImGui::BeginMenu("File", true))
      {
         if (ImGui::MenuItem("Save Configuration"))
         {
            std::cout << "save camera system config" << std::endl;
            config.serializeToFile();
         }

         if (ImGui::MenuItem("Reload Configuration"))
         {
            std::cout << "reload camera system config" << std::endl;
            config.deserializeFromFile();
         }

         ImGui::EndMenu();
      }
      ImGui::EndMainMenuBar();
   }

   if (ImGui::CollapsingHeader("horizontal", header_flags))
   {
      drawFloatElement("camera velocity factor", &config._camera_velocity_factor_x, -0.1f, 10.0f);
      drawFloatElement("focus zone divider", &config._focus_zone_divider, 0.1f, 100.0f);
      drawFloatElement("target shift factor", &config._target_shift_factor, 0.1f, 2.0f);
      drawIntElement("back in bounds tolerance", &config._back_in_bounds_tolerance_x, 1, 50);
      ImGui::Checkbox("follow player orientation", &config._follow_player_orientation);
   }

   if (ImGui::CollapsingHeader("vertical", header_flags))
   {
      drawFloatElement("camera velocity factor", &config._camera_velocity_factor_y, 0.1f, 10.0f);
      drawFloatElement("view ratio", &config._view_ratio_y, 0.1f, 10.0f);
      drawIntElement("back in bounds tolerance", &config._back_in_bounds_tolerance_y, 1, 50);
      drawIntElement("player offset", &config._player_offset_y, -500, 500);
      drawFloatElement("panic line divider", &config._panic_line_divider, 0.1f, 100.0f);
      drawFloatElement("panic acceleration factor", &config._panic_acceleration_factor_y, 0.1f, 10.0f);
   }

   if (ImGui::CollapsingHeader("various", header_flags))
   {
      ImGui::Checkbox("camera shaking", &config._camera_shaking_enabled);
   }

   ImGui::End();

   _render_window->clear();
   ImGui::SFML::Render(*_render_window.get());
   _render_window->display();
}

void CameraSystemConfigurationUi::close()
{
   ImGui::SFML::Shutdown();
}
