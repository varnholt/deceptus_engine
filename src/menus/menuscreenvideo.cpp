#include "menus/menuscreenvideo.h"

#include "framework/tools/localization.h"
#include "framework/tools/sfmlstring.h"
#include "game/config/gameconfiguration.h"
#include "game/config/gpupreference.h"
#include "menus/menu.h"
#include "menus/menuaudio.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <utility>

#ifdef DECEPTUS_VRSFML
#include <SFML/Window/VideoMode.hpp>
#include <SFML/Window/VideoModeUtils.hpp>
#endif

static const auto STEP_SIZE = 10;

namespace
{

//! the states the scaling row cycles through, in order, as {pixel precision, keep aspect} pairs. a
//! whole number scale is applied to both axes alike and so keeps the aspect ratio by construction,
//! which is why the last entry sets both flags rather than leaving the config in the one combination
//! that reads as a contradiction
constexpr std::array<std::pair<bool, bool>, 3> scaling_states{{{false, false}, {false, true}, {true, true}}};

int32_t currentScalingStateIndex()
{
   const auto& config = GameConfiguration::getInstance();

   for (auto index = 0; index < static_cast<int32_t>(scaling_states.size()); index++)
   {
      if (scaling_states[index].first == config._preserve_pixel_precision && scaling_states[index].second == config._preserve_aspect_ratio)
      {
         return index;
      }
   }

   // a hand edited config can ask for pixel precision without keeping the aspect ratio, which comes out
   // as pixel precision either way, so that is the state the row reports
   return config._preserve_pixel_precision ? static_cast<int32_t>(scaling_states.size()) - 1 : 0;
}

}  // namespace

MenuScreenVideo::MenuScreenVideo()
{
   setFilename("data/menus/video.psd");

   _base_video_modes = {{640, 360}, {1280, 720}, {1366, 768}, {1600, 900}, {1920, 1080}, {2560, 1440}, {3840, 2160}};

#ifdef DECEPTUS_VRSFML
   const auto desktop_mode = sf::VideoModeUtils::getDesktopMode();
#else
   const auto desktop_mode = sf::VideoMode::getDesktopMode();
#endif
   std::erase_if(
      _base_video_modes,
      [&desktop_mode](const std::array<int32_t, 2>& mode)
      { return mode[0] > static_cast<int32_t>(desktop_mode.size.x) || mode[1] > static_cast<int32_t>(desktop_mode.size.y); }
   );

   refreshVideoModes();
}

void MenuScreenVideo::refreshVideoModes()
{
   // the active resolution is not necessarily one of the predefined modes: going fullscreen adopts the
   // desktop mode and resizing the window by dragging its border yields arbitrary sizes. keep the active
   // resolution in the list so it can be found and stepped away from in either direction.
   _video_modes = _base_video_modes;

   const std::array<int32_t, 2> current_mode{
      GameConfiguration::getInstance()._video_mode_width, GameConfiguration::getInstance()._video_mode_height
   };

   if (std::find(_video_modes.begin(), _video_modes.end(), current_mode) != _video_modes.end())
   {
      return;
   }

   // std::array compares lexicographically, so this keeps the list ordered by width, then height
   _video_modes.insert(std::lower_bound(_video_modes.begin(), _video_modes.end(), current_mode), current_mode);
}

bool MenuScreenVideo::isRowAvailable(Selection selection) const
{
   if (selection == Selection::GpuPreference)
   {
      return ::GpuPreference::isSupported();
   }

   return true;
}

void MenuScreenVideo::up()
{
   auto next = static_cast<int32_t>(_selection);
   do
   {
      next--;
      if (next < 0)
      {
         next = static_cast<int32_t>(Selection::Count) - 1;
      }
   } while (!isRowAvailable(static_cast<Selection>(next)));

   _selection = static_cast<Selection>(next);
   updateLayers();

   MenuAudio::play(MenuAudio::SoundEffect::ItemNavigate);
}

void MenuScreenVideo::down()
{
   auto next = static_cast<int32_t>(_selection);
   do
   {
      next++;
      if (next == static_cast<int32_t>(Selection::Count))
      {
         next = 0;
      }
   } while (!isRowAvailable(static_cast<Selection>(next)));

   _selection = static_cast<Selection>(next);
   updateLayers();

   MenuAudio::play(MenuAudio::SoundEffect::ItemNavigate);
}

void MenuScreenVideo::select(int32_t step)
{
   switch (_selection)
   {
      case Selection::DisplayMode:
      {
         _fullscreen_callback();
         break;
      }

      case Selection::Resolution:
      {
         // in fullscreen the resolution follows the desktop mode, so the row is a readout and cannot be changed
         if (GameConfiguration::getInstance()._fullscreen)
         {
            return;
         }

         refreshVideoModes();

         auto it = std::find_if(
            std::begin(_video_modes),
            std::end(_video_modes),
            [](const std::array<int32_t, 2> arr) {
               return arr[0] == GameConfiguration::getInstance()._video_mode_width &&
                      arr[1] == GameConfiguration::getInstance()._video_mode_height;
            }
         );

         const auto current_index = static_cast<int32_t>(it - _video_modes.begin());
         const auto new_index = std::clamp(current_index + (step < 0 ? -1 : 1), 0, static_cast<int32_t>(_video_modes.size()) - 1);
         if (new_index != current_index)
         {
            _resolution_callback(_video_modes[new_index][0], _video_modes[new_index][1]);
         }
         break;
      }

      case Selection::Brightness:
      {
         float brightness = GameConfiguration::getInstance()._brightness;
         brightness += (0.01f * step);

         if (brightness < 0.0f)
         {
            brightness = 0.0f;
         }
         else if (brightness > 1.0f)
         {
            brightness = 1.0f;
         }

         GameConfiguration::getInstance()._brightness = brightness;
         break;
      }

      case Selection::VSync:
      {
         GameConfiguration::getInstance()._vsync_enabled = !GameConfiguration::getInstance()._vsync_enabled;
         _vsync_callback();
         break;
      }

      case Selection::Scaling:
      {
         const auto state_count = static_cast<int32_t>(scaling_states.size());
         const auto next_index = (currentScalingStateIndex() + (step < 0 ? state_count - 1 : 1)) % state_count;

         GameConfiguration::getInstance()._preserve_pixel_precision = scaling_states[next_index].first;
         GameConfiguration::getInstance()._preserve_aspect_ratio = scaling_states[next_index].second;

         _scaling_callback();
         break;
      }

      case Selection::GpuPreference:
      {
         // the enumerators are numbered in the order the row cycles through them
         constexpr auto preference_count = 3;
         const auto current = static_cast<int32_t>(::GpuPreference::read());
         const auto next = (current + (step < 0 ? preference_count - 1 : 1)) % preference_count;
         ::GpuPreference::write(static_cast<::GpuPreference::Preference>(next));
         break;
      }

      case Selection::Count:
      {
         break;
      }
   }

   GameConfiguration::getInstance().serializeToFile();
   updateLayers();

   MenuAudio::play(MenuAudio::SoundEffect::ItemTick);
}

void MenuScreenVideo::back()
{
   Menu::getInstance()->show(Menu::MenuType::Options);
   MenuAudio::play(MenuAudio::SoundEffect::MenuBack);
}

void MenuScreenVideo::setFullscreenCallback(MenuScreenVideo::FullscreenCallback callback)
{
   _fullscreen_callback = callback;
}

void MenuScreenVideo::setResolutionCallback(MenuScreenVideo::ResolutionCallback callback)
{
   _resolution_callback = callback;
}

void MenuScreenVideo::setVSyncCallback(VSyncCallback callback)
{
   _vsync_callback = callback;
}

void MenuScreenVideo::setScalingCallback(ScalingCallback callback)
{
   _scaling_callback = callback;
}

void MenuScreenVideo::keyboardKeyPressed(sf::Keyboard::Key key)
{
   if (key == sf::Keyboard::Key::Up)
   {
      up();
   }

   else if (key == sf::Keyboard::Key::Down)
   {
      down();
   }

   else if (key == sf::Keyboard::Key::Left)
   {
      select(-STEP_SIZE);
   }

   else if (key == sf::Keyboard::Key::Right)
   {
      select(STEP_SIZE);
   }

   else if (key == sf::Keyboard::Key::Escape)
   {
      back();
   }
}

void MenuScreenVideo::loadingFinished()
{
   for (auto index = 0; index < 11; index++)
   {
      const auto brightness_value_layer_name = std::format("brightness_value_{}", index);
      _brightness_value_layers.push_back(_layers[brightness_value_layer_name]);
   }

   // hide all PSD resolution value layers — replaced by dynamic text
   for (const auto& layer_name :
        {"resolution_value_1024x576",
         "resolution_value_1280x720",
         "resolution_value_1366x768",
         "resolution_value_1536x864",
         "resolution_value_1600x900",
         "resolution_value_1920x1080",
         "resolution_value_3840x2160"})
   {
      if (_layers.contains(layer_name))
      {
         _layers[layer_name]->_visible = false;
      }
   }

   // read reference rects from PSD text layers before hiding them
   _row_label_base_rect = _layers["resolution_text_0"]->_sprite->getGlobalBounds();
   _row_help_base_rect = _layers["resolution_help"]->_sprite->getGlobalBounds();
   _row_value_base_rect = _layers["displayMode_value_windowed"]->_sprite->getGlobalBounds();
   _row_stride = _layers["displayMode_text_0"]->_sprite->getGlobalBounds().position.y - _row_label_base_rect.position.y;

   for (const auto& layer_name :
        {"resolution_text_0",
         "resolution_text_1",
         "resolution_help",
         "displayMode_text_0",
         "displayMode_text_1",
         "displayMode_help",
         "displayMode_value_windowed",
         "displayMode_value_borderless",
         "displayMode_value_fullscreen",
         "vSync_text_0",
         "vSync_text_1",
         "vSync_help",
         "vSync_value_0",
         "vSync_value_1",
         "brightness_text_0",
         "brightness_text_1",
         "brightness_help"})
   {
      _layers[layer_name]->_visible = false;
   }

   auto make_label = [this]() -> std::unique_ptr<sf::Text>
   {
#ifdef DECEPTUS_VRSFML
      auto text = std::make_unique<sf::Text>(_font, sf::Text::Data{});
#else
      auto text = std::make_unique<sf::Text>(_font);
#endif
      text->setFont(_font);
      text->setCharacterSize(12);
      return text;
   };

   _resolution_text = make_label();
   _resolution_text->setFillColor(sf::Color::White);
#ifdef DECEPTUS_VRSFML
   _resolution_text->position = {382, 154};
#else
   _resolution_text->setPosition({382, 154});
#endif

   _resolution_label = make_label();
   _resolution_help_text = make_label();
   _resolution_help_text->setFillColor(color_help_text);

   _displaymode_label = make_label();
   _displaymode_help_text = make_label();
   _displaymode_help_text->setFillColor(color_help_text);
   _displaymode_value_text = make_label();
   _displaymode_value_text->setFillColor(sf::Color::White);

   _vsync_label = make_label();
   _vsync_help_text = make_label();
   _vsync_help_text->setFillColor(color_help_text);
   _vsync_value_text = make_label();
   _vsync_value_text->setFillColor(sf::Color::White);

   _brightness_label = make_label();
   _brightness_help_text = make_label();
   _brightness_help_text->setFillColor(color_help_text);

   _scaling_label = make_label();
   _scaling_help_text = make_label();
   _scaling_help_text->setFillColor(color_help_text);
   _scaling_value_text = make_label();
   _scaling_value_text->setFillColor(sf::Color::White);

   _gpu_label = make_label();
   _gpu_help_text = make_label();
   _gpu_help_text->setFillColor(color_help_text);
   _gpu_value_text = make_label();
   _gpu_value_text->setFillColor(sf::Color::White);

   _text_back_button = make_label();
   _text_back_button->setFillColor(color_label_normal);
   _text_defaults_button = make_label();
   _text_defaults_button->setFillColor(color_label_normal);

   updateLayers();
}

void MenuScreenVideo::draw(sf::RenderTarget& window, sf::RenderStates states)
{
   MenuScreen::draw(window, states);

   if (!_resolution_label)
   {
      return;
   }

   window.draw(*_resolution_label, states);
   if (_selection == Selection::Resolution)
   {
      window.draw(*_resolution_help_text, states);
   }
   window.draw(*_resolution_text, states);

   window.draw(*_displaymode_label, states);
   if (_selection == Selection::DisplayMode)
   {
      window.draw(*_displaymode_help_text, states);
   }
   window.draw(*_displaymode_value_text, states);

   window.draw(*_vsync_label, states);
   if (_selection == Selection::VSync)
   {
      window.draw(*_vsync_help_text, states);
   }
   window.draw(*_vsync_value_text, states);

   window.draw(*_brightness_label, states);
   if (_selection == Selection::Brightness)
   {
      window.draw(*_brightness_help_text, states);
   }

   window.draw(*_scaling_label, states);
   if (_selection == Selection::Scaling)
   {
      window.draw(*_scaling_help_text, states);
   }
   window.draw(*_scaling_value_text, states);

   if (isRowAvailable(Selection::GpuPreference))
   {
      window.draw(*_gpu_label, states);
      if (_selection == Selection::GpuPreference)
      {
         window.draw(*_gpu_help_text, states);
      }
      window.draw(*_gpu_value_text, states);
   }

   window.draw(*_text_back_button, states);
   window.draw(*_text_defaults_button, states);
}

void MenuScreenVideo::updateLayers()
{
   refreshVideoModes();

   const auto resolution_selected = _selection == Selection::Resolution;
   const auto display_mode_selected = _selection == Selection::DisplayMode;
   const auto vsync_selected = _selection == Selection::VSync;
   const auto brightness_selected = _selection == Selection::Brightness;
   const auto scaling_selected = _selection == Selection::Scaling;
   const auto gpu_selected = _selection == Selection::GpuPreference;

   auto display_mode_value_index = 0;

   const auto fullscreen = GameConfiguration::getInstance()._fullscreen;
   if (fullscreen)
   {
      display_mode_value_index = 2;
   }

   // the resolution is dictated by the desktop mode while fullscreen, so the row turns into a readout
   const auto resolution_editable = !fullscreen;

   const auto resolution_width = GameConfiguration::getInstance()._video_mode_width;
   const auto resolution_height = GameConfiguration::getInstance()._video_mode_height;

   const auto brightness_value = GameConfiguration::getInstance()._brightness;
   const auto vsync_enabled = GameConfiguration::getInstance()._vsync_enabled;

   _layers["defaults_xbox_0"]->_visible = isControllerUsed();
   _layers["defaults_xbox_1"]->_visible = false;
   _layers["back_xbox_0"]->_visible = isControllerUsed();
   _layers["back_xbox_1"]->_visible = false;

   _layers["defaults_pc_0"]->_visible = !isControllerUsed();
   _layers["defaults_pc_1"]->_visible = false;
   _layers["back_pc_0"]->_visible = !isControllerUsed();
   _layers["back_pc_1"]->_visible = false;

   _layers["resolution_highlight"]->_visible = resolution_selected;
   _layers["resolution_arrows"]->_visible = resolution_selected && resolution_editable;

   _layers["brightness_body_0"]->_visible = !brightness_selected;
   _layers["brightness_body_1"]->_visible = brightness_selected;
   _layers["brightness_highlight"]->_visible = brightness_selected;
   _layers["brightness_arrows"]->_visible = brightness_selected;
   _layers["brightness_h_0"]->_visible = !brightness_selected;
   _layers["brightness_h_1"]->_visible = brightness_selected;

#ifdef DECEPTUS_VRSFML
   _layers["brightness_h_0"]->_sprite->origin = {50 - (brightness_value * 100.0f), 0};
   _layers["brightness_h_1"]->_sprite->origin = {50 - (brightness_value * 100.0f), 0};
#else
   _layers["brightness_h_0"]->_sprite->setOrigin({50 - (brightness_value * 100.0f), 0});
   _layers["brightness_h_1"]->_sprite->setOrigin({50 - (brightness_value * 100.0f), 0});
#endif

   _layers["displayMode_highlight"]->_visible = display_mode_selected;
   _layers["displayMode_arrows"]->_visible = display_mode_selected;

   // the psd carries highlight and arrow art for the four rows it was drawn with, so the scaling and gpu
   // rows have none of their own. the vsync pair is the same shape they need - a bar with a value and
   // arrows either side - so it is shifted down onto whichever of them is selected and sits back on its
   // own row otherwise. only ever one row is highlighted, so a single pair covers all three
   auto highlighted_row = static_cast<int32_t>(Selection::VSync);
   if (scaling_selected)
   {
      highlighted_row = static_cast<int32_t>(Selection::Scaling);
   }
   else if (gpu_selected)
   {
      highlighted_row = static_cast<int32_t>(Selection::GpuPreference);
   }

   // a sprite is drawn at its position minus its origin, so a negative origin shifts it down the screen
   const auto highlight_shift = static_cast<float>(highlighted_row - static_cast<int32_t>(Selection::VSync)) * _row_stride;

   _layers["vSync_highlight"]->_visible = vsync_selected || scaling_selected || gpu_selected;
   _layers["vSync_arrows"]->_visible = _layers["vSync_highlight"]->_visible;

#ifdef DECEPTUS_VRSFML
   _layers["vSync_highlight"]->_sprite->origin = {0.0f, -highlight_shift};
   _layers["vSync_arrows"]->_sprite->origin = {0.0f, -highlight_shift};
#else
   _layers["vSync_highlight"]->_sprite->setOrigin({0.0f, -highlight_shift});
   _layers["vSync_arrows"]->_sprite->setOrigin({0.0f, -highlight_shift});
#endif

   const auto brightness_index = static_cast<int32_t>(std::ceil((brightness_value * 10.0f) - 0.1f));
   for (auto index = 0; index < 11; index++)
   {
      _brightness_value_layers[index]->_visible = (index == brightness_index);
   }

   if (!_resolution_label)
   {
      return;
   }

   // the psd's help line sits just below the five rows it was drawn for, so a sixth row lands on top
   // of it. it is one shared line for every row, so moving it down by one row keeps them apart
   auto help_rect = _row_help_base_rect;
   if (isRowAvailable(Selection::GpuPreference))
   {
      help_rect.position.y += _row_stride;
   }

   // resolution row
   _resolution_label->setString(sftr("Resolution"));
   if (resolution_editable)
   {
      _resolution_label->setFillColor(resolution_selected ? color_label_selected : color_label_normal);
   }
   else
   {
      _resolution_label->setFillColor(color_help_text);
   }
   placeTextLeft(*_resolution_label, rowRect(_row_label_base_rect, 0));

   _resolution_help_text->setString(
      resolution_editable ? sftr("Set the display resolution") : sftr("The display resolution is used while in fullscreen mode")
   );
   placeTextCentered(*_resolution_help_text, help_rect);

#ifdef DECEPTUS_VRSFML
   _resolution_text->setString(std::format("{}x{}", resolution_width, resolution_height).c_str());
#else
   _resolution_text->setString(std::format("{}x{}", resolution_width, resolution_height));
#endif

   // display mode row
   _displaymode_label->setString(sftr("Display Mode"));
   _displaymode_label->setFillColor(display_mode_selected ? color_label_selected : color_label_normal);
   placeTextLeft(*_displaymode_label, rowRect(_row_label_base_rect, 1));

   _displaymode_help_text->setString(sftr("Change the display render mode of the game"));

   placeTextCentered(*_displaymode_help_text, help_rect);

#ifdef DECEPTUS_VRSFML
   const sf::Utf8String display_mode_strings[] = {sftr("Windowed"), sftr("Borderless"), sftr("Fullscreen")};
#else
   const sf::String display_mode_strings[] = {sftr("Windowed"), sftr("Borderless"), sftr("Fullscreen")};
#endif
   _displaymode_value_text->setString(display_mode_strings[display_mode_value_index]);
   placeTextLeft(*_displaymode_value_text, rowRect(_row_value_base_rect, 0));

   // vsync row
   _vsync_label->setString(sftr("V-Sync"));
   _vsync_label->setFillColor(vsync_selected ? color_label_selected : color_label_normal);
   placeTextLeft(*_vsync_label, rowRect(_row_label_base_rect, 2));

   _vsync_help_text->setString(sftr("Adjust the Vertical Synchronization"));

   placeTextCentered(*_vsync_help_text, help_rect);

   _vsync_value_text->setString(vsync_enabled ? sftr("On") : sftr("Off"));
   placeTextLeft(*_vsync_value_text, rowRect(_row_value_base_rect, 1));

   // brightness row
   _brightness_label->setString(sftr("Brightness"));
   _brightness_label->setFillColor(brightness_selected ? color_label_selected : color_label_normal);
   placeTextLeft(*_brightness_label, rowRect(_row_label_base_rect, 3));

   _brightness_help_text->setString(sftr("Adjust the screen brightness"));

   placeTextCentered(*_brightness_help_text, help_rect);

   // scaling row
   _scaling_label->setString(sftr("Scaling"));
   _scaling_label->setFillColor(scaling_selected ? color_label_selected : color_label_normal);
   placeTextLeft(*_scaling_label, rowRect(_row_label_base_rect, static_cast<int32_t>(Selection::Scaling)));

   _scaling_help_text->setString(sftr("Set how the image is fitted into the window"));
   placeTextCentered(*_scaling_help_text, help_rect);

#ifdef DECEPTUS_VRSFML
   const sf::Utf8String scaling_strings[] = {sftr("Stretch"), sftr("Keep Aspect"), sftr("Pixel Precision")};
#else
   const sf::String scaling_strings[] = {sftr("Stretch"), sftr("Keep Aspect"), sftr("Pixel Precision")};
#endif
   _scaling_value_text->setString(scaling_strings[currentScalingStateIndex()]);
   placeTextLeft(*_scaling_value_text, rowRect(_row_value_base_rect, static_cast<int32_t>(Selection::Scaling) - 1));

   // gpu row
   if (isRowAvailable(Selection::GpuPreference))
   {
      _gpu_label->setString(sftr("Graphics Card"));
      _gpu_label->setFillColor(gpu_selected ? color_label_selected : color_label_normal);
      placeTextLeft(*_gpu_label, rowRect(_row_label_base_rect, static_cast<int32_t>(Selection::GpuPreference)));

      _gpu_help_text->setString(sftr("Pick the graphics card to run on, applies after a restart"));
      placeTextCentered(*_gpu_help_text, help_rect);

#ifdef DECEPTUS_VRSFML
      // the value box is only as wide as the psd drew it, so these have to stay about as short as
      // the longest of the other rows ("Pixel Precision")
      const sf::Utf8String gpu_strings[] = {sftr("Automatic"), sftr("Performance"), sftr("Power Saving")};
#else
      const sf::String gpu_strings[] = {sftr("Automatic"), sftr("Performance"), sftr("Power Saving")};
#endif
      _gpu_value_text->setString(gpu_strings[static_cast<int32_t>(::GpuPreference::read())]);
      placeTextLeft(*_gpu_value_text, rowRect(_row_value_base_rect, static_cast<int32_t>(Selection::GpuPreference) - 1));
   }

   const auto& back_layer = isControllerUsed() ? _layers["back_xbox_0"] : _layers["back_pc_0"];
   _text_back_button->setString(sftr("Back"));
   placeTextRightOf(*_text_back_button, back_layer->_sprite->getGlobalBounds());

   const auto& defaults_layer = isControllerUsed() ? _layers["defaults_xbox_0"] : _layers["defaults_pc_0"];
   _text_defaults_button->setString(sftr("Defaults"));
   placeTextRightOf(*_text_defaults_button, defaults_layer->_sprite->getGlobalBounds());
}

/*
data/menus/video.psd

   bg_temp

   video-window-bg
   video_window-main

   header
*/
