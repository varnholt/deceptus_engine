#include <imgui-SFML.h>
#include <imgui.h>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>
#include <cstdint>
#include <iostream>
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

   // Load the test_menu texture and create a sprite
   sf::Texture testMenuTexture;
   if (!testMenuTexture.loadFromFile("data/textures/test_menu.png"))
   {
      std::cout << "Could not load test_menu.png texture" << std::endl;
      return 1;
   }

   sf::Sprite testMenuSprite(testMenuTexture);

   // Set the sprite to cover the entire render window
   sf::Vector2u textureSize = testMenuTexture.getSize();
   testMenuSprite.setScale(
      {static_cast<float>(renderMode.size.x) / static_cast<float>(textureSize.x),
       static_cast<float>(renderMode.size.y) / static_cast<float>(textureSize.y)}
   );

   // Initial viewport for 3D window
   glViewport(0, 0, static_cast<GLsizei>(renderMode.size.x), static_cast<GLsizei>(renderMode.size.y));

   while (renderWindow.isOpen() && editorWindow.isOpen())
   {
      // event loop for renderer
      while (auto event = renderWindow.pollEvent())
      {
         if (event->is<sf::Event::Closed>())
         {
            renderWindow.close();
         }
         else if (auto* resize = event->getIf<sf::Event::Resized>())
         {
            glViewport(0, 0, static_cast<GLsizei>(resize->size.x), static_cast<GLsizei>(resize->size.y));

            renderWindow.setView(
               sf::View(sf::FloatRect({0.f, 0.f}, {static_cast<float>(resize->size.x), static_cast<float>(resize->size.y)}))
            );

            // Notify the mechanism about the resize so it can update the camera's aspect ratio
            mechanism.resize(static_cast<int>(resize->size.x), static_cast<int>(resize->size.y));
         }
      }

      // event loop for editor
      while (auto event = editorWindow.pollEvent())
      {
         ImGui::SFML::ProcessEvent(editorWindow, *event);

         if (event->is<sf::Event::Closed>())
         {
            editorWindow.close();
         }
      }

      sf::Time dt = clock.restart();

      ImGui::SFML::Update(editorWindow, dt);

      // draw 3d scene
      if (renderWindow.setActive(true))
      {
         glEnable(GL_DEPTH_TEST);
         glDepthFunc(GL_LESS);
         glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
         glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

         mechanism.update(dt);
         mechanism.draw(renderWindow, renderWindow);

         glDisable(GL_DEPTH_TEST);
         glEnable(GL_BLEND);
         glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

         glBindVertexArray(0);
         glBindBuffer(GL_ARRAY_BUFFER, 0);
         glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
         glUseProgram(0);
         renderWindow.pushGLStates();
         renderWindow.resetGLStates();  // Reset SFML states to defaults
         renderWindow.draw(testMenuSprite);
         renderWindow.popGLStates();
         renderWindow.display();
      }

      // draw editor
      if (editorWindow.setActive(true))
      {
         // glBindVertexArray(0);
         // glBindBuffer(GL_ARRAY_BUFFER, 0);
         // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
         // glUseProgram(0);

         glDisable(GL_DEPTH_TEST);
         editorWindow.clear(sf::Color(50, 50, 50));
         mechanism.drawEditor();
         ImGui::SFML::Render(editorWindow);
         editorWindow.display();
      }
   }

   ImGui::SFML::Shutdown();
   return 0;
}
