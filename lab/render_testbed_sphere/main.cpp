#include <imgui-SFML.h>
#include <imgui.h>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>
#include <cstdint>
#include "testmechanism.h"

// Include OpenGL headers for glViewport, VAOs, etc.
#include "opengl/gl_current.h"  // make sure this pulls in glew + gl.h

int main()
{
   std::string title = "3D VBO Testbed";

   // Set up OpenGL context settings for 3D rendering
   sf::ContextSettings contextSettings;
   contextSettings.depthBits = 24;  // enable depth buffer
   contextSettings.stencilBits = 8;
   contextSettings.attributeFlags = sf::ContextSettings::Default;
   contextSettings.majorVersion = 3;
   contextSettings.minorVersion = 3;

   // sf::RenderWindow imguiWindow(sf::VideoMode(400, 300), "ImGui Tools");

   sf::RenderWindow window;
   sf::VideoMode mode({1280, 720});
   window.create(mode, title, static_cast<uint32_t>(sf::Style::Default), sf::State::Windowed, contextSettings);

   window.setVerticalSyncEnabled(true);

   // --- OpenGL / GLEW init order -----------------------------------------
   // make sure the SFML context is current before touching GL
   if (!window.setActive(true))
   {
      return 1;
   }

   // GLEW first
   glewExperimental = GL_TRUE;
   if (glewInit() != GLEW_OK)
   {
      return 1;
   }

   // Now ImGui-SFML: this will create font texture, etc.
   if (!ImGui::SFML::Init(window))
   {
      return 1;
   }

   TestMechanism mechanism;
   sf::Clock clock;

   // Initial viewport
   glViewport(0, 0, static_cast<GLsizei>(mode.size.x), static_cast<GLsizei>(mode.size.y));

   while (window.isOpen())
   {
      // --------------------------------------------------------------------
      // Event loop
      // --------------------------------------------------------------------
      while (auto event = window.pollEvent())
      {
         ImGui::SFML::ProcessEvent(window, *event);

         if (event->is<sf::Event::Closed>())
         {
            window.close();
         }
         else if (auto* resize = event->getIf<sf::Event::Resized>())
         {
            glViewport(0, 0, static_cast<GLsizei>(resize->size.x), static_cast<GLsizei>(resize->size.y));

            window.setView(sf::View(sf::FloatRect({0.f, 0.f}, {static_cast<float>(resize->size.x), static_cast<float>(resize->size.y)})));

            // Notify the mechanism about the resize so it can update the camera's aspect ratio
            mechanism.resize(static_cast<int>(resize->size.x), static_cast<int>(resize->size.y));
         }
      }

      sf::Time dt = clock.restart();

      // ImGui logic / IO
      ImGui::SFML::Update(window, dt);

      // Your own logic
      mechanism.update(dt);

      // --------------------------------------------------------------------
      // OpenGL state reset BEFORE ImGui draws
      // This avoids imgui-sfml crashing when a VAO / buffers are still bound
      // --------------------------------------------------------------------
      glBindVertexArray(0);
      glBindBuffer(GL_ARRAY_BUFFER, 0);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
      glUseProgram(0);

      // Build ImGui UI
      mechanism.drawEditor();  // assumes this only calls ImGui::XXX

      // Let ImGui-SFML render its draw lists (uses glDrawElements internally)
      ImGui::SFML::Render(window);

      // Now draw your 3D stuff. It should fully set up whatever state it needs.
      mechanism.draw(window, window);

      window.display();
   }

   ImGui::SFML::Shutdown();
   return 0;
}
