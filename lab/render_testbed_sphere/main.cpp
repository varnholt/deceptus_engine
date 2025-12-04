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

namespace
{
sf::ContextSettings _context_settings;

sf::RenderWindow _render_window;
sf::VideoMode _render_mode({1280, 720});

sf::RenderWindow _editor_window;
sf::VideoMode _editor_mode({400, 300});

std::unique_ptr<TestMechanism> _mechanism;
sf::Clock _clock;

std::unique_ptr<sf::Sprite> _test_menu_sprite;
}  // namespace

void drawScene(const sf::Time& delta_time)
{
   if (!_render_window.setActive(true))
   {
      return;
   }

   // Get the current render window size and set viewport before any 3D rendering
   sf::Vector2u window_size = _render_window.getSize();
   glViewport(0, 0, static_cast<GLsizei>(window_size.x), static_cast<GLsizei>(window_size.y));

   glEnable(GL_DEPTH_TEST);
   glDepthFunc(GL_LESS);
   glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   _mechanism->update(delta_time);
   _mechanism->draw(_render_window, _render_window);

   glDisable(GL_DEPTH_TEST);
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   glBindVertexArray(0);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
   glUseProgram(0);

   _render_window.pushGLStates();
   _render_window.resetGLStates();  // Reset SFML states to defaults
   _render_window.draw(*_test_menu_sprite);
   _render_window.popGLStates();
   _render_window.display();
}

void drawEditor()
{
   if (!_editor_window.setActive(true))
   {
      return;
   }
   // glBindVertexArray(0);
   // glBindBuffer(GL_ARRAY_BUFFER, 0);
   // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
   // glUseProgram(0);

   glDisable(GL_DEPTH_TEST);
   _editor_window.clear(sf::Color(50, 50, 50));
   _mechanism->drawEditor();
   ImGui::SFML::Render(_editor_window);
   _editor_window.display();
}

void eventLoopRenderer()
{
   while (auto event = _render_window.pollEvent())
   {
      if (event->is<sf::Event::Closed>())
      {
         _render_window.close();
      }
      else if (const auto* resize_event = event->getIf<sf::Event::Resized>())
      {
         glViewport(0, 0, static_cast<GLsizei>(resize_event->size.x), static_cast<GLsizei>(resize_event->size.y));

         _render_window.setView(
            sf::View(sf::FloatRect({0.f, 0.f}, {static_cast<float>(resize_event->size.x), static_cast<float>(resize_event->size.y)}))
         );

         _test_menu_sprite->setScale(
            {static_cast<float>(resize_event->size.x) / static_cast<float>(_render_mode.size.x),
             static_cast<float>(resize_event->size.y / static_cast<float>(_render_mode.size.y))}
         );

         // Notify the mechanism about the resize so it can update the camera's aspect ratio
         _mechanism->resize(static_cast<int>(resize_event->size.x), static_cast<int>(resize_event->size.y));
      }
   }
}

void eventLoopEditor()
{
   while (auto event = _editor_window.pollEvent())
   {
      ImGui::SFML::ProcessEvent(_editor_window, *event);

      if (event->is<sf::Event::Closed>())
      {
         _editor_window.close();
      }
   }
}

int main()
{
   std::string title = "3D VBO Testbed";
   std::string editorTitle = "Editor Tools";

   // set up opengl context settings
   _context_settings.depthBits = 24;  // enable depth buffer
   _context_settings.stencilBits = 8;
   _context_settings.attributeFlags = sf::ContextSettings::Default;
   _context_settings.majorVersion = 3;
   _context_settings.minorVersion = 3;

   _render_window.create(_render_mode, title, static_cast<uint32_t>(sf::Style::Default), sf::State::Windowed, _context_settings);
   _render_window.setVerticalSyncEnabled(true);

   _editor_window.create(_editor_mode, editorTitle, static_cast<uint32_t>(sf::Style::Default), sf::State::Windowed);
   _editor_window.setVerticalSyncEnabled(true);
   _editor_window.setPosition(
      {_render_window.getPosition().x + static_cast<int32_t>(_render_window.getSize().x), _render_window.getPosition().y}
   );

   // make sure the 3D rendering window's context is current before intializing glew
   if (!_render_window.setActive(true))
   {
      return 1;
   }

   glewExperimental = GL_TRUE;
   if (glewInit() != GLEW_OK)
   {
      return 1;
   }

   // make editor window's context current before initializing it
   if (!_editor_window.setActive(true))
   {
      return 1;
   }

   if (!ImGui::SFML::Init(_editor_window))
   {
      return 1;
   }

   // load test_menu texture and create a sprite
   if (!_render_window.setActive(true))
   {
      return 1;
   }

   _mechanism = std::make_unique<TestMechanism>();

   sf::Texture test_menu_texture;
   if (!test_menu_texture.loadFromFile("data/textures/test_menu.png"))
   {
      std::cout << "Could not load test_menu.png texture\n";
      return 1;
   }

   _test_menu_sprite = std::make_unique<sf::Sprite>(test_menu_texture);

   // Set the sprite to cover the entire render window
   sf::Vector2u texture_size = test_menu_texture.getSize();
   _test_menu_sprite->setScale(
      {static_cast<float>(_render_mode.size.x) / static_cast<float>(texture_size.x),
       static_cast<float>(_render_mode.size.y) / static_cast<float>(texture_size.y)}
   );

   // set up viewport
   glViewport(0, 0, static_cast<GLsizei>(_render_mode.size.x), static_cast<GLsizei>(_render_mode.size.y));

   // Load saved values after all initialization is done
   _mechanism->loadValues();

   while (_render_window.isOpen())
   {
      eventLoopRenderer();
      eventLoopEditor();
      sf::Time delta_time = _clock.restart();

      if (_editor_window.isOpen())
      {
         ImGui::SFML::Update(_editor_window, delta_time);
      }

      drawScene(delta_time);
      drawEditor();
   }

   ImGui::SFML::Shutdown();
   return 0;
}
