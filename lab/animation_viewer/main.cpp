#include <imgui-SFML.h>
#include <imgui.h>
#include <SFML/Graphics.hpp>
#include <iostream>
#include <memory>
#include "game/animation/animation.h"
#include "game/animation/animationpool.h"

namespace
{
int32_t selected_index = -1;
}

int main()
{
   sf::RenderWindow window(sf::VideoMode(800, 600), "Animation Viewer");
   window.setFramerateLimit(60);

   if (!ImGui::SFML::Init(window))
   {
      return 1;
   }

   // Load animations from the animation pool
   AnimationPool animation_pool("animations.json");
   try
   {
      animation_pool.initialize();
   }
   catch (const std::exception& e)
   {
      std::cerr << "Failed to initialize AnimationPool: " << e.what() << std::endl;
      return 1;
   }

   for (auto& [k, v] : animation_pool.settings())
   {
      animation_pool.create(k /*, 0.0f, 0.0f, true, true*/);
   }

   const auto& animations = animation_pool.getAnimations();
   std::vector<std::string> animation_names;
   animation_names.reserve(animations.size());

   for (const auto& [name, animation] : animations)
   {
      animation_names.push_back(name);
   }

   std::shared_ptr<Animation> current_animation = nullptr;
   auto playing = true;

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

      sf::Time delta_time = delta_clock.restart();
      if (current_animation && playing)
      {
         current_animation->update(delta_time);
      }

      ImGui::SFML::Update(window, delta_time /*deltaClock.restart()*/);
      ImGui::Begin("Animation Controls");

      if (ImGui::Button("|>"))
      {
         if (current_animation)
         {
            current_animation->play();
            playing = true;
         }
      }

      ImGui::SameLine();

      if (ImGui::Button("||"))
      {
         if (current_animation)
         {
            current_animation->pause();
            playing = false;
         }
      }

      ImGui::SameLine();

      if (ImGui::Button("[]"))
      {
         if (current_animation)
         {
            current_animation->stop();
            playing = false;
         }
      }

      ImGui::Separator();

      if (ImGui::BeginCombo(
             "Available Animations",
             (selected_index >= 0 && selected_index < static_cast<int>(animation_names.size())) ? animation_names[selected_index].c_str()
                                                                                                : "Select an animation"
          ))
      {
         for (int i = 0; i < static_cast<int>(animation_names.size()); ++i)
         {
            bool is_selected = (i == selected_index);
            if (ImGui::Selectable(animation_names[i].c_str(), is_selected))
            {
               selected_index = i;
               auto it = animations.find(animation_names[i]);
               if (it != animations.end())
               {
                  current_animation = it->second;
                  current_animation->play();
                  playing = true;
               }
            }

            if (is_selected)
            {
               ImGui::SetItemDefaultFocus();
            }
         }

         ImGui::EndCombo();
      }

      // handle keyboard input on combobox
      if (ImGui::IsItemFocused())
      {
         auto update_animation = [&](int new_index)
         {
            selected_index = new_index;
            auto it = animations.find(animation_names[selected_index]);
            if (it != animations.end())
            {
               current_animation = it->second;
               current_animation->play();
               playing = true;
            }
         };

         if (ImGui::IsKeyPressed(ImGuiKey_UpArrow) && selected_index > 0)
         {
            update_animation(selected_index - 1);
         }
         if (ImGui::IsKeyPressed(ImGuiKey_DownArrow) && selected_index < static_cast<int>(animation_names.size()) - 1)
         {
            update_animation(selected_index + 1);
         }
      }

      ImGui::End();

      // draw animation
      const auto clear_color = sf::Color(50, 50, 80, 255);
      window.clear(clear_color);

      constexpr auto scale = 5.0f;
      if (current_animation)
      {
         current_animation->setPosition(current_animation->_frames[0].width * scale, current_animation->_frames[0].height * scale);
         current_animation->setScale(scale, scale);
         current_animation->draw(window);
      }

      ImGui::SFML::Render(window);
      window.display();
   }

   ImGui::SFML::Shutdown();
   return 0;
}
