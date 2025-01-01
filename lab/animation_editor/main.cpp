#include "editor.h"

#include <imgui-SFML.h>
#include <imgui.h>

int main()
{
   sf::RenderWindow window(sf::VideoMode(800, 600), "Animation Viewer");
   window.setFramerateLimit(60);

   if (!ImGui::SFML::Init(window))
   {
      return 1;
   }

   Editor editor;

   if (!editor.init())
   {
      return 1;
   }

   sf::Clock delta_clock;

   while (window.isOpen())
   {
      sf::Event event;
      while (window.pollEvent(event))
      {
         ImGui::SFML::ProcessEvent(event);
         if (event.type == sf::Event::Closed)
         {
            window.close();
         }
      }

      const auto delta_time = delta_clock.restart();
      editor.update(delta_time);
      ImGui::SFML::Update(window, delta_time);

      // draw animation and settings
      window.clear();
      editor.draw(window);
      ImGui::SFML::Render(window);
      window.display();
   }

   ImGui::SFML::Shutdown();

   // editor.loop();
   return 0;
}
