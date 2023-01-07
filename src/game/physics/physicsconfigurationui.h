#ifndef PHYSICSCONFIGURATIONUI_H
#define PHYSICSCONFIGURATIONUI_H

#include <memory>
#include "SFML/Graphics.hpp"

class PhysicsConfigurationUi
{
public:
   PhysicsConfigurationUi();

   void processEvents();
   void draw();
   void close();

   std::unique_ptr<sf::RenderWindow> _render_window;
   sf::Clock _clock;
};

#endif // PHYSICSCONFIGURATIONUI_H
