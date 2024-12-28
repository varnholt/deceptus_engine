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

void drawCheckerboardGrid(sf::RenderWindow& window, float cell_size = 16.0f)
{
   const sf::Vector2u window_size = window.getSize();
   const sf::Color grey(128, 128, 128, 255);
   const sf::Color dark_grey(96, 96, 96, 255);

   const auto rows = static_cast<int>(window_size.y / cell_size) + 1;
   const auto cols = static_cast<int>(window_size.x / cell_size) + 1;

   sf::RectangleShape cell(sf::Vector2f(cell_size, cell_size));

   for (auto y = 0; y < rows; ++y)
   {
      for (auto x = 0; x < cols; ++x)
      {
         cell.setFillColor(((x + y) % 2 == 0) ? grey : dark_grey);
         cell.setPosition(x * cell_size, y * cell_size);
         window.draw(cell);
      }
   }
}

void editAnimationSettings(AnimationSettings& settings)
{
   ImGui::Separator();
   ImGui::Text("Edit Animation Settings");

   // Edit frame size
   if (ImGui::InputInt2("Frame Size", settings._frame_size.data()))
   {
      // Handle updates to frame size here if needed
   }

   // Edit frame offset
   if (ImGui::InputInt2("Frame Offset", settings._frame_offset.data()))
   {
      // Handle updates to frame offset here if needed
   }

   // Edit origin
   if (ImGui::InputFloat2("Origin", settings._origin.data()))
   {
      // Handle updates to origin here if needed
   }

   // Edit sprite count
   if (ImGui::InputInt("Sprite Count", &settings._sprite_count))
   {
      // Handle updates to sprite count here if needed
   }

   // Edit texture path
   char texture_path[256] = {};
   std::strncpy(texture_path, settings._texture_path.string().c_str(), sizeof(texture_path) - 1);
   if (ImGui::InputText("Texture Path", texture_path, sizeof(texture_path)))
   {
      settings._texture_path = std::filesystem::path(texture_path);
   }

   // Edit frame durations
   if (ImGui::TreeNode("Frame Durations"))
   {
      for (size_t i = 0; i < settings._frame_durations.size(); ++i)
      {
         float duration = settings._frame_durations[i].asSeconds();
         if (ImGui::InputFloat(("Duration " + std::to_string(i)).c_str(), &duration))
         {
            settings._frame_durations[i] = sf::seconds(duration);
         }
      }

      if (ImGui::Button("Add Frame Duration"))
      {
         settings._frame_durations.emplace_back(sf::seconds(0.1f));  // Default value
      }

      if (!settings._frame_durations.empty() && ImGui::Button("Remove Last Frame Duration"))
      {
         settings._frame_durations.pop_back();
      }

      ImGui::TreePop();
   }
}

}  // namespace

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

      ImGui::SameLine();

      if (ImGui::Button("<<"))
      {
         if (current_animation && current_animation->_current_frame > 0)
         {
            current_animation->_current_frame--;
            current_animation->_elapsed = current_animation->getFrameTimes()[current_animation->_current_frame];
            current_animation->updateVertices();
         }
      }

      ImGui::SameLine();

      if (ImGui::Button(">>"))
      {
         if (current_animation && current_animation->_current_frame < static_cast<int32_t>(current_animation->_frames.size()) - 1)
         {
            current_animation->_current_frame++;
            current_animation->_elapsed = current_animation->getFrameTimes()[current_animation->_current_frame];
            current_animation->updateVertices();
         }
      }

      if (ImGui::Button("Save"))
      {
         animation_pool.saveToJson();
      }

      ImGui::SameLine();

      if (ImGui::Button("Reload"))
      {
         animation_pool.reloadFromJson();
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

      drawCheckerboardGrid(window);

      constexpr auto scale = 5.0f;
      if (current_animation)
      {
         // Calculate the rectangle dimensions
         sf::FloatRect rect{0, 0, current_animation->_frames[0].width * scale, current_animation->_frames[0].height * scale};

         // Draw the background rectangle
         sf::RectangleShape rs;
         rs.setSize({rect.width, rect.height});
         rs.setPosition(rect.left, rect.top);
         rs.setFillColor(sf::Color(0, 0, 0, 100));
         window.draw(rs);

         sf::Vector2f origin = current_animation->getOrigin();
         current_animation->setPosition(rect.left + origin.x * scale, rect.top + origin.y * scale);
         current_animation->setScale(scale, scale);

         // Draw the animation
         current_animation->draw(window);

         ImGui::Separator();
         ImGui::Text("Current Animation: %s", animation_names[selected_index].c_str());

         // Edit the settings of the current animation
         auto settings_it = animation_pool.settings().find(animation_names[selected_index]);
         if (settings_it != animation_pool.settings().end())
         {
            editAnimationSettings(*settings_it->second);
         }
      }

      ImGui::SFML::Render(window);
      window.display();
   }

   ImGui::SFML::Shutdown();
   return 0;
}
