#ifndef CAMERASYSTEMCONFIGURATIONUI_H
#define CAMERASYSTEMCONFIGURATIONUI_H

#include <memory>
#include "SFML/Graphics.hpp"

/// \brief provides an ImGui editor window for live camera configuration tuning.
class CameraSystemConfigurationUi
{
public:
   /// \brief creates the configuration window and initializes ImGui-SFML bindings.
   CameraSystemConfigurationUi();

   /// \brief processes window and ImGui events, including close requests.
   void processEvents();
   /// \brief draws the configuration controls and applies edited values to the shared config.
   void draw();
   /// \brief shuts down ImGui-SFML resources used by the configuration window.
   void close();

   std::unique_ptr<sf::RenderWindow> _render_window;
   sf::Clock _clock;
};

#endif  // CAMERASYSTEMCONFIGURATIONUI_H
