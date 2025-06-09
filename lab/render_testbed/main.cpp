#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>
#include "testmechanism.h"

int main()
{
   std::string title = "render testbed";

   sf::RenderWindow window;
   sf::VideoMode mode({1280, 720});
   window.create(mode, title);

   TestMechanism mechanism;
   sf::Clock clock;

   while (window.isOpen())
   {
      while (auto event = window.pollEvent())
      {
         if (event->is<sf::Event::Closed>())
         {
            window.close();
         }
         else if (auto* resize = event->getIf<sf::Event::Resized>())
         {
            window.setView(sf::View(sf::FloatRect({0.f, 0.f}, {static_cast<float>(resize->size.x), static_cast<float>(resize->size.y)})));
         }
      }

      sf::Time dt = clock.restart();
      mechanism.update(dt);

      window.clear();
      mechanism.draw(window, window);
      window.display();
   }

   return 0;
}
