#include "camerasystemconfigurationui.h"

#include "camerasystemconfiguration.h"
#include "game/debug/settingsui.h"

#pragma warning(push, 0)
#include "imgui/imgui-SFML.h"
#include "imgui/imgui.h"
#pragma warning(pop)

#include <iostream>
#include <sstream>

CameraSystemConfigurationUi::CameraSystemConfigurationUi()
#ifdef DECEPTUS_VRSFML
    : _render_window(std::make_unique<sf::RenderWindow>(
         sf::RenderWindow::create(sf::WindowSettings{.size = {800u, 620u}, .title = "deceptus camera configuration"}).value()
      ))
#else
    : _render_window(std::make_unique<sf::RenderWindow>(sf::VideoMode({800, 620}), "deceptus camera configuration"))
#endif
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
#ifndef DECEPTUS_VRSFML
         _render_window->close();
#endif
      }
   }
}

void CameraSystemConfigurationUi::draw()
{
   auto& config = CameraSystemConfiguration::getInstance();

   ImGui::SFML::Update(*_render_window.get(), _clock.restart());

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

   const auto* viewport = ImGui::GetMainViewport();
   ImGui::SetNextWindowPos(viewport->WorkPos);
   ImGui::SetNextWindowSize(viewport->WorkSize);
   ImGui::Begin(
      "camera", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar
   );
   ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.50f);
   ImGuiTreeNodeFlags header_flags = ImGuiTreeNodeFlags_DefaultOpen;

   SettingsUi::beginSettings();

   // scopes the widget ids per section, so rows sharing a label stay distinct
   if (ImGui::CollapsingHeader("horizontal", header_flags))
   {
      ImGui::PushID("horizontal");
      SettingsUi::drawFloat(
         "camera velocity factor",
         &config._camera_velocity_factor_x,
         -0.1f,
         10.0f,
         "How quickly the camera closes the gap to where it is aiming. It covers this fraction of the "
         "remaining distance every second, so higher is snappier and lower drifts.",
         "This also sets how long the camera takes to reach a running player: roughly 3 divided by this, "
         "in seconds. The lead factor below is measured against it, so changing this changes the lead too."
      );
      SettingsUi::drawFloat(
         "camera lead factor",
         &config._camera_lead_factor_x,
         0.0f,
         1.0f,
         "How far ahead of the player the camera aims, as a fraction of the distance it would need in "
         "order to travel at exactly the player's speed.",
         "0 aims straight at the player, which leaves the camera standing still whenever a room edge "
         "lets go of it. 1 asks for the whole distance, which cancels the easing and welds the camera to "
         "the player. Higher values also drift a little further past the player when they stop."
      );
      SettingsUi::drawFloat(
         "focus zone divider",
         &config._focus_zone_divider,
         0.1f,
         100.0f,
         "Sets the dead zone the player can move inside without the camera following: the view width "
         "divided by this, to either side of centre.",
         "Larger means a smaller dead zone, so 100 is very nearly no dead zone at all and the camera "
         "follows almost any movement. It also scales the target shift factor below."
      );
      SettingsUi::drawFloat(
         "target shift factor",
         &config._target_shift_factor,
         0.1f,
         2.0f,
         "How far the dead zone slides in the direction the player is facing, as a fraction of its own "
         "half width, so the camera shows more of what is ahead.",
         "Has no effect unless follow player orientation is on. Because it is a fraction of the dead "
         "zone, a large focus zone divider leaves nothing for it to shift."
      );
      SettingsUi::drawInt(
         "back in bounds tolerance",
         &config._back_in_bounds_tolerance_x,
         1,
         50,
         "How close to centre the camera has to get before it stops following again, in pixels. It is "
         "what keeps the camera from chasing every last pixel.",
         "Only bites while it is smaller than the dead zone above. At a focus zone divider of 100 the "
         "dead zone is about 6 px, so anything above that never takes effect."
      );
      SettingsUi::drawBool(
         "follow player orientation",
         &config._follow_player_orientation,
         "Slides the dead zone towards the direction the player is facing, so turning around shifts the "
         "view rather than leaving the player centred.",
         "Turning it on is what gives the target shift factor above something to do."
      );
      ImGui::PopID();
   }

   if (ImGui::CollapsingHeader("vertical", header_flags))
   {
      ImGui::PushID("vertical");
      SettingsUi::drawFloat(
         "camera velocity factor",
         &config._camera_velocity_factor_y,
         0.1f,
         10.0f,
         "How quickly the camera closes the vertical gap to the player, as a fraction of the remaining "
         "distance per second.",
         "The vertical follow also eases in over its first moments rather than starting at full speed, "
         "so this is the speed it works up to, not the speed it starts at."
      );
      SettingsUi::drawFloat(
         "view ratio",
         &config._view_ratio_y,
         0.1f,
         10.0f,
         "Where the player sits vertically: one view height divided by this, measured down from the top "
         "edge of the view.",
         "2 centres the player. Smaller values push them further down the screen, so 1.5 leaves them two "
         "thirds of the way down and shows more of what is above."
      );
      SettingsUi::drawInt(
         "back in bounds tolerance",
         &config._back_in_bounds_tolerance_y,
         1,
         50,
         "How close to its resting height the camera has to get before it stops following vertically, in "
         "pixels.",
         "Also what ends panic mode's grip once the player is back near the middle of the view."
      );
      SettingsUi::drawInt(
         "player offset",
         &config._player_offset_y,
         -500,
         500,
         "Shifts the point the camera tracks this far below the player, in pixels.",
         "A positive value moves the view down, so the player sits higher on screen and more of what is "
         "below them is visible. It also moves the panic lines with it, since they are measured against "
         "the same point."
      );
      SettingsUi::drawFloat(
         "panic line divider",
         &config._panic_line_divider,
         0.1f,
         100.0f,
         "Sets how far the player may leave the middle of the view while airborne before the camera "
         "starts chasing them: the view height divided by this, above and below centre.",
         "Larger means the lines sit closer to centre, so the camera panics sooner. Normally the camera "
         "does not follow at all while the player is in the air; this is what overrides that."
      );
      SettingsUi::drawFloat(
         "panic acceleration factor",
         &config._panic_acceleration_factor_y,
         0.1f,
         10.0f,
         "How much faster than usual the camera follows vertically once it is panicking.",
         "Multiplies the vertical velocity factor, and replaces the gentle ease-in the normal follow "
         "uses, so it takes hold immediately."
      );
      ImGui::PopID();
   }

   if (ImGui::CollapsingHeader("various", header_flags))
   {
      SettingsUi::drawBool(
         "camera shaking",
         &config._camera_shaking_enabled,
         "Whether explosions and other impacts shake the view.",
         "Off silences every shake at the source, so nothing else has to be tuned to stop it."
      );
   }

   SettingsUi::endSettings();

   ImGui::End();

   _render_window->clear();
   ImGui::SFML::Render(*_render_window.get());
   _render_window->display();
}

void CameraSystemConfigurationUi::close()
{
   ImGui::SFML::Shutdown(*_render_window.get());
}
