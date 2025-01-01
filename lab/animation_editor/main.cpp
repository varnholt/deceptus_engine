#include "editor.h"

#include <imgui-SFML.h>
#include <imgui.h>

int main()
{
   sf::RenderWindow window(sf::VideoMode(1600, 900), "deceptus animation editor");
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

   ImGui::LoadIniSettingsFromDisk("settings.ini");

   sf::Clock delta_clock;

   while (window.isOpen())
   {
      // process events
      sf::Event event;
      while (window.pollEvent(event))
      {
         ImGui::SFML::ProcessEvent(event);
         if (event.type == sf::Event::Closed)
         {
            window.close();
         }
      }

      // update
      const auto delta_time = delta_clock.restart();
      editor.update(delta_time);
      ImGui::SFML::Update(window, delta_time);

      // draw
      window.clear();
      editor.draw(window);
      ImGui::SFML::Render(window);
      window.display();
   }

   ImGui::SaveIniSettingsToDisk("settings.ini");
   ImGui::SFML::Shutdown();
   return 0;
}
