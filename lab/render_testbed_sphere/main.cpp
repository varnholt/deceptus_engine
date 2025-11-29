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
   std::string editorTitle = "Editor Tools";

   // Set up OpenGL context settings for 3D rendering
   sf::ContextSettings contextSettings;
   contextSettings.depthBits = 24;  // enable depth buffer
   contextSettings.stencilBits = 8;
   contextSettings.attributeFlags = sf::ContextSettings::Default;
   contextSettings.majorVersion = 3;
   contextSettings.minorVersion = 3;

   // Create main 3D rendering window
   sf::RenderWindow renderWindow;
   sf::VideoMode renderMode({1280, 720});
   renderWindow.create(renderMode, title, static_cast<uint32_t>(sf::Style::Default), sf::State::Windowed, contextSettings);
   renderWindow.setVerticalSyncEnabled(true);

   // Create separate window for ImGui editor
   sf::RenderWindow editorWindow;
   sf::VideoMode editorMode({400, 300});
   editorWindow.create(editorMode, editorTitle, static_cast<uint32_t>(sf::Style::Default), sf::State::Windowed);
   editorWindow.setVerticalSyncEnabled(true);

   // --- OpenGL / GLEW init order -----------------------------------------
   // Make sure the 3D rendering window's context is current before touching GL
   if (!renderWindow.setActive(true))
   {
      return 1;
   }

   // GLEW first
   glewExperimental = GL_TRUE;
   if (glewInit() != GLEW_OK)
   {
      return 1;
   }

   // Initialize ImGui-SFML for the editor window
   // Need to switch to editor window context for ImGui initialization
   if (!editorWindow.setActive(true))
   {
      return 1;
   }

   if (!ImGui::SFML::Init(editorWindow))
   {
      return 1;
   }

   // Switch context back to render window for 3D rendering
   if (!renderWindow.setActive(true))
   {
      return 1;
   }

   TestMechanism mechanism;
   sf::Clock clock;

   // Initial viewport for 3D window
   glViewport(0, 0, static_cast<GLsizei>(renderMode.size.x), static_cast<GLsizei>(renderMode.size.y));

   while (renderWindow.isOpen() && editorWindow.isOpen())
   {
      // --------------------------------------------------------------------
      // Event loop for 3D rendering window
      // --------------------------------------------------------------------
      while (auto event = renderWindow.pollEvent())
      {
         if (event->is<sf::Event::Closed>())
         {
            renderWindow.close();
         }
         else if (auto* resize = event->getIf<sf::Event::Resized>())
         {
            glViewport(0, 0, static_cast<GLsizei>(resize->size.x), static_cast<GLsizei>(resize->size.y));

            renderWindow.setView(sf::View(sf::FloatRect({0.f, 0.f}, {static_cast<float>(resize->size.x), static_cast<float>(resize->size.y)})));

            // Notify the mechanism about the resize so it can update the camera's aspect ratio
            mechanism.resize(static_cast<int>(resize->size.x), static_cast<int>(resize->size.y));
         }
      }

      // --------------------------------------------------------------------
      // Event loop for editor window
      // --------------------------------------------------------------------
      while (auto event = editorWindow.pollEvent())
      {
         ImGui::SFML::ProcessEvent(editorWindow, *event);

         if (event->is<sf::Event::Closed>())
         {
            editorWindow.close();
         }
      }

      sf::Time dt = clock.restart();

      // Update ImGui with the editor window's delta time
      ImGui::SFML::Update(editorWindow, dt);

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

      // Make sure the rendering context is active for 3D rendering
      if (!renderWindow.setActive(true))
      {
         return 1;
      }

      // Now draw your 3D stuff in the render window. It should fully set up whatever state it needs.
      mechanism.draw(renderWindow, renderWindow);

      // Switch to the editor window context to draw the UI
      if (!editorWindow.setActive(true))
      {
         return 1;
      }

      // Clear the editor window for proper rendering
      editorWindow.clear(sf::Color(50, 50, 50));  // Dark gray background

      // Build ImGui UI in the editor window
      mechanism.drawEditor();  // assumes this only calls ImGui::XXX

      // Let ImGui-SFML render its draw lists in the editor window
      ImGui::SFML::Render(editorWindow);
      editorWindow.display();

      // Switch back to render window and display
      if (!renderWindow.setActive(true))
      {
         return 1;
      }
      renderWindow.display();
   }

   ImGui::SFML::Shutdown();
   return 0;
}
