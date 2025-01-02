#include "editor.h"
// #include "imgui_internal.h"

#include <imgui-SFML.h>
#include <imgui.h>
#include <SFML/Graphics.hpp>
#include <iostream>
#include <memory>
#include "game/animation/animation.h"
#include "game/animation/animationpool.h"

namespace
{
constexpr auto scale = 5.0f;
}

void Editor::drawAnimation(sf::RenderTarget& window)
{
   // scale to maintain aspect ratio
   const auto& frame = _current_animation->_frames[0];
   const auto target_width = frame.width * scale;
   const auto target_height = frame.height * scale;

   sf::FloatRect rect{0, 0, target_width, target_height};
   sf::RectangleShape rs;
   rs.setSize({rect.width, rect.height});
   rs.setPosition(rect.left, rect.top);
   rs.setFillColor(sf::Color(0, 0, 0, 100));
   window.draw(rs);

   sf::Vector2f origin = _current_animation->getOrigin();
   _current_animation->setPosition(rect.left + origin.x * scale, rect.top + origin.y * scale);
   _current_animation->setScale(scale, scale);
   _current_animation->draw(window);

   ImGui::Separator();
   ImGui::Text("Current Animation: %s", _animation_names[_selected_index.value()].c_str());
}

void Editor::selectAnimByCursorKey()
{
   if (!_selected_index.has_value())
   {
      _selected_index = 0;
   }

   const auto selected_index_prev = _selected_index;

   if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
   {
      (*_selected_index)--;
   }

   if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
   {
      (*_selected_index)++;
   }

   _selected_index = std::clamp(_selected_index.value(), 0, static_cast<int32_t>(_animation_names.size() - 1));

   if (_selected_index != selected_index_prev)
   {
      selectAnimation(_selected_index.value());
   }
}

void Editor::populateComboBox()
{
   const auto& animations = _animation_pool->getAnimations();

   for (auto i = 0; i < static_cast<int32_t>(_animation_names.size()); ++i)
   {
      const auto is_selected = (i == _selected_index);
      if (ImGui::Selectable(_animation_names[i].c_str(), is_selected))
      {
         _selected_index = i;
         const auto it = animations.find(_animation_names[i]);
         if (it != animations.end())
         {
            assignCurrentAnimation(it->second);
            _current_animation->play();
            _playing = true;
         }
      }

      if (is_selected)
      {
         ImGui::SetItemDefaultFocus();
      }
   }
}

void Editor::drawControls()
{
   ImGui::Begin("Animation Controls");

   if (ImGui::Button("▶"))
   {
      if (_current_animation)
      {
         _current_animation->play();
         _playing = true;
      }
   }

   ImGui::SameLine();

   if (ImGui::Button("⏸"))
   {
      if (_current_animation)
      {
         _current_animation->pause();
         _playing = false;
      }
   }

   ImGui::SameLine();

   if (ImGui::Button("⏹"))
   {
      if (_current_animation)
      {
         _current_animation->stop();
         _playing = false;
      }
   }

   ImGui::SameLine();

   if (ImGui::Button("⏮"))
   {
      if (_current_animation && _current_animation->_current_frame > 0)
      {
         _current_animation->_current_frame--;
         _current_animation->_elapsed = _current_animation->getFrameTimes()[_current_animation->_current_frame];
         _current_animation->updateVertices();
      }
   }

   ImGui::SameLine();

   if (ImGui::Button("⏭"))
   {
      if (_current_animation && _current_animation->_current_frame < static_cast<int32_t>(_current_animation->_frames.size()) - 1)
      {
         _current_animation->_current_frame++;
         _current_animation->_elapsed = _current_animation->getFrameTimes()[_current_animation->_current_frame];
         _current_animation->updateVertices();
      }
   }

   ImGui::SameLine();

   if (ImGui::Checkbox("loop", &_loop))
   {
   }

   if (ImGui::Button("Save"))
   {
      _animation_pool->saveToJson();
   }

   ImGui::SameLine();

   if (ImGui::Button("Reload"))
   {
      _animation_pool->reloadFromJson();
   }

   ImGui::Separator();

   if (ImGui::BeginCombo(
          "Available Animations", (_selected_index.has_value()) ? _animation_names[_selected_index.value()].c_str() : "Select animation"
       ))
   {
      populateComboBox();
      ImGui::EndCombo();
   }

   // handle keyboard input on combobox
   if (ImGui::IsItemFocused())
   {
      selectAnimByCursorKey();
   }

   ImGui::End();
}

void Editor::update(const sf::Time& delta_time)
{
   if (_current_animation && _playing)
   {
      _current_animation->update(delta_time);

      if (_loop && _current_animation->_paused)
      {
         _current_animation->seekToStart();
         _current_animation->play();
      }
   }
}

void Editor::draw(sf::RenderTarget& window)
{
   drawControls();

   // draw animation and settings
   drawCheckerboardGrid(window, 16.0f * scale);

   if (_current_animation)
   {
      drawAnimation(window);
      drawAnimationSettings();
   }
}

void Editor::loadAnimationNames()
{
   const auto& animations = _animation_pool->getAnimations();
   _animation_names.clear();
   _animation_names.reserve(animations.size());
   for (const auto& [name, animation] : animations)
   {
      _animation_names.push_back(name);
   }
}

bool Editor::init()
{
   // Load animations from the animation pool
   _animation_pool = std::make_unique<AnimationPool>("animations.json");
   try
   {
      _animation_pool->initialize();
   }
   catch (const std::exception& e)
   {
      std::cerr << "Failed to initialize AnimationPool: " << e.what() << std::endl;
      return false;
   }

   for (auto& [k, v] : _animation_pool->settings())
   {
      _animation_pool->create(k);
   }

   loadAnimationNames();

   // add support for player icons
   static const ImWchar audio_icon_ranges[] = {
      0x23ED,
      0x23EE,  // ⏭, ⏮
      0x23F8,
      0x23F9,  // ⏸, ⏹
      0x25B6,
      0x25B6,  // ▶
      0
   };
   ImGuiIO& io = ImGui::GetIO();
   ImFontConfig fontConfig;
   fontConfig.MergeMode = true;  // merge with default font
   fontConfig.PixelSnapH = true;
   io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/seguisym.ttf", 16.0f, &fontConfig, audio_icon_ranges);
   io.Fonts->Build();
   return ImGui::SFML::UpdateFontTexture();
}

void Editor::drawCheckerboardGrid(sf::RenderTarget& window, float base_cell_size)
{
   if (_current_animation == nullptr)
   {
      return;
   }

   const sf::Vector2u window_size = window.getSize();

   // adjust cell dimensions to match the frame's aspect ratio
   const auto& frame = _current_animation->_frames[0];
   const auto cell_width = base_cell_size;
   const auto cell_height = base_cell_size;

   const sf::Color grey(128, 128, 128, 255);
   const sf::Color dark_grey(96, 96, 96, 255);

   const auto rows = static_cast<int32_t>(window_size.y / cell_height) + 1;
   const auto cols = static_cast<int32_t>(window_size.x / cell_width) + 1;

   sf::RectangleShape cell(sf::Vector2f(cell_width, cell_height));

   for (auto y = 0; y < rows; ++y)
   {
      for (auto x = 0; x < cols; ++x)
      {
         cell.setFillColor(((x + y) % 2 == 0) ? grey : dark_grey);
         cell.setPosition(x * cell_width, y * cell_height);
         window.draw(cell);
      }
   }
}

void Editor::drawAnimationSettings()
{
   ImGui::Separator();
   ImGui::Text("Edit Animation Settings");

   std::optional<AnimationPool::UpdateFlag> update_flag;

   if (ImGui::InputInt2("Frame Size", _current_settings->_frame_size.data()))
   {
      update_flag = AnimationPool::UpdateFlag::Settings;
   }

   if (ImGui::InputInt2("Frame Offset", _current_settings->_frame_offset.data()))
   {
      update_flag = AnimationPool::UpdateFlag::Settings;
   }

   if (ImGui::InputFloat2("Origin", _current_settings->_origin.data()))
   {
      update_flag = AnimationPool::UpdateFlag::Settings;
   }

   char texture_path[1024] = {};
   std::strncpy(texture_path, _current_settings->_texture_path.string().c_str(), sizeof(texture_path) - 1);
   if (ImGui::InputText("Texture Path", texture_path, sizeof(texture_path)))
   {
      _current_settings->_texture_path = std::filesystem::path(texture_path);
      update_flag = AnimationPool::UpdateFlag::All;
   }

   if (ImGui::TreeNode("Frame Durations"))
   {
      for (size_t i = 0; i < _current_settings->_frame_durations.size(); ++i)
      {
         float duration = _current_settings->_frame_durations[i].asSeconds();
         if (ImGui::InputFloat(("Duration " + std::to_string(i)).c_str(), &duration))
         {
            _current_settings->_frame_durations[i] = sf::seconds(duration);
            update_flag = AnimationPool::UpdateFlag::Settings;
         }
      }

      if (ImGui::Button("[+]"))
      {
         _current_settings->_frame_durations.emplace_back(sf::seconds(0.1f));
         update_flag = AnimationPool::UpdateFlag::Settings;
      }

      ImGui::SameLine();

      if (!_current_settings->_frame_durations.empty() && ImGui::Button("[-]"))
      {
         _current_settings->_frame_durations.pop_back();
         update_flag = AnimationPool::UpdateFlag::Settings;
      }

      ImGui::TreePop();
   }

   if (update_flag.has_value() && !_reloaded)
   {
      _reloaded = true;
      _animation_pool->recreateAnimationsFromSettings(update_flag.value());
      loadAnimationNames();
   }
   else
   {
      _reloaded = false;
   }
}

void Editor::assignCurrentAnimation(const std::shared_ptr<Animation>& animation)
{
   _current_animation = animation;

   auto settings_it = _animation_pool->settings().find(_current_animation->_name);
   if (settings_it != _animation_pool->settings().end())
   {
      _current_settings = settings_it->second;
   }
}

void Editor::selectAnimation(int32_t index)
{
   _selected_index = index;
   const auto& animations = _animation_pool->getAnimations();
   const auto it = animations.find(_animation_names[_selected_index.value()]);
   if (it != animations.end())
   {
      assignCurrentAnimation(it->second);
      _current_animation->play();
      _playing = true;
   }
}
