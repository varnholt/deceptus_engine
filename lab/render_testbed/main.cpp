#include <imgui-SFML.h>
#include <imgui.h>
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

   if (!ImGui::SFML::Init(window))
   {
      return 1;
   }

   TestMechanism mechanism;
   sf::Clock clock;

   while (window.isOpen())
   {
      while (auto event = window.pollEvent())
      {
         ImGui::SFML::ProcessEvent(window, *event);
         if (event->is<sf::Event::Closed>())
         {
            window.close();
         }
         else if (auto* resize = event->getIf<sf::Event::Resized>())
         {
            window.setView(sf::View(sf::FloatRect({0.f, 0.f}, {static_cast<float>(resize->size.x), static_cast<float>(resize->size.y)})));
         }
         else if (auto* key = event->getIf<sf::Event::KeyPressed>())
         {
            if (key->code == sf::Keyboard::Key::Space)
            {
               mechanism.chooseNextState();
            }
         }
      }

      sf::Time dt = clock.restart();
      ImGui::SFML::Update(window, dt);

      mechanism.update(dt);

      window.clear();
      mechanism.draw(window, window);
      ImGui::SFML::Render(window);
      window.display();
   }

   ImGui::SFML::Shutdown();
   return 0;
}
