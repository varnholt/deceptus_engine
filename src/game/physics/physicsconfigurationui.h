#ifndef PHYSICSCONFIGURATIONUI_H
#define PHYSICSCONFIGURATIONUI_H

#include <memory>
#include "SFML/Graphics.hpp"

/// \brief imgui-based tool window for inspecting and editing physics configuration values at runtime.
class PhysicsConfigurationUi
{
public:
   /// \brief creates the ui window and initializes imgui-sfml integration.
   PhysicsConfigurationUi();

   /// \brief polls sfml events, forwards them to imgui, and handles window close requests.
   void processEvents();
   /// \brief renders configuration controls and applies edits directly to the singleton config.
   void draw();
   /// \brief shuts down imgui-sfml resources.
   void close();

   std::unique_ptr<sf::RenderWindow> _render_window;
   sf::Clock _clock;
};

#endif  // PHYSICSCONFIGURATIONUI_H
