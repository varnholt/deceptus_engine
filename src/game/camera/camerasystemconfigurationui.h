#ifndef CAMERASYSTEMCONFIGURATIONUI_H
#define CAMERASYSTEMCONFIGURATIONUI_H

#include <memory>
#include "SFML/Graphics.hpp"

class CameraSystemConfigurationUi
{
public:
   CameraSystemConfigurationUi();

   void processEvents();
   void draw();
   void close();

   std::unique_ptr<sf::RenderWindow> _render_window;
   sf::Clock _clock;
};

#endif  // CAMERASYSTEMCONFIGURATIONUI_H
