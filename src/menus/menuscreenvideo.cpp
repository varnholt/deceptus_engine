#include "menus/menuscreenvideo.h"

#include "framework/tools/localization.h"
#include "framework/tools/sfmlstring.h"
#include "game/config/gameconfiguration.h"
#include "menus/menu.h"
#include "menus/menuaudio.h"

#include <cmath>
#include <format>

static const auto STEP_SIZE = 10;

MenuScreenVideo::MenuScreenVideo()
{
   setFilename("data/menus/video.psd");

   _video_modes = {{640, 360}, {1280, 720}, {1366, 768}, {1600, 900}, {1920, 1080}, {2560, 1440}, {3840, 2160}};

   const auto desktop_mode = sf::VideoMode::getDesktopMode();
   std::erase_if(
      _video_modes,
      [&desktop_mode](const std::array<int32_t, 2>& mode)
      { return mode[0] > static_cast<int32_t>(desktop_mode.size.x) || mode[1] > static_cast<int32_t>(desktop_mode.size.y); }
   );
}

void MenuScreenVideo::up()
{
   auto next = static_cast<int32_t>(_selection);
   next--;
   if (next < 0)
   {
      next = static_cast<int32_t>(Selection::Count) - 1;
   }

   _selection = static_cast<Selection>(next);
   updateLayers();

   MenuAudio::play(MenuAudio::SoundEffect::ItemNavigate);
}

void MenuScreenVideo::down()
{
   auto next = static_cast<int32_t>(_selection);
   next++;
   if (next == static_cast<int32_t>(Selection::Count))
   {
      next = 0;
   }

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

   ensureFontLoaded();

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
      auto text = std::make_unique<sf::Text>(_font);
      text->setFont(_font);
      text->setCharacterSize(12);
      return text;
   };

   _resolution_text = make_label();
   _resolution_text->setFillColor(sf::Color::White);
   _resolution_text->setPosition({382, 154});

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

   window.draw(*_text_back_button, states);
   window.draw(*_text_defaults_button, states);
}

void MenuScreenVideo::updateLayers()
{
   const auto resolution_selected = _selection == Selection::Resolution;
   const auto display_mode_selected = _selection == Selection::DisplayMode;
   const auto vsync_selected = _selection == Selection::VSync;
   const auto brightness_selected = _selection == Selection::Brightness;

   auto resolution_index = 0u;
   auto display_mode_value_index = 0;

   const auto fullscreen = GameConfiguration::getInstance()._fullscreen;
   if (fullscreen)
   {
      display_mode_value_index = 2;
   }

   const auto resolution_width = GameConfiguration::getInstance()._video_mode_width;
   const auto resolution_height = GameConfiguration::getInstance()._video_mode_height;

   for (auto index = 0u; index < _video_modes.size(); index++)
   {
      if (_video_modes[index][0] == resolution_width && _video_modes[index][1] == resolution_height)
      {
         resolution_index = index;
         break;
      }
   }

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
   _layers["resolution_arrows"]->_visible = resolution_selected;

   _layers["brightness_body_0"]->_visible = !brightness_selected;
   _layers["brightness_body_1"]->_visible = brightness_selected;
   _layers["brightness_highlight"]->_visible = brightness_selected;
   _layers["brightness_arrows"]->_visible = brightness_selected;
   _layers["brightness_h_0"]->_visible = !brightness_selected;
   _layers["brightness_h_1"]->_visible = brightness_selected;

   _layers["brightness_h_0"]->_sprite->setOrigin({50 - (brightness_value * 100.0f), 0});
   _layers["brightness_h_1"]->_sprite->setOrigin({50 - (brightness_value * 100.0f), 0});

   _layers["displayMode_highlight"]->_visible = display_mode_selected;
   _layers["displayMode_arrows"]->_visible = display_mode_selected;

   _layers["vSync_highlight"]->_visible = vsync_selected;
   _layers["vSync_arrows"]->_visible = vsync_selected;

   const auto brightness_index = static_cast<int32_t>(std::ceil((brightness_value * 10.0f) - 0.1f));
   for (auto index = 0; index < 11; index++)
   {
      _brightness_value_layers[index]->_visible = (index == brightness_index);
   }

   if (!_resolution_label)
   {
      return;
   }

   // resolution row
   _resolution_label->setString(sftr("Resolution"));
   _resolution_label->setFillColor(resolution_selected ? color_label_selected : color_label_normal);
   placeTextLeft(*_resolution_label, rowRect(_row_label_base_rect, 0));

   _resolution_help_text->setString(sftr("Set the display resolution"));
   placeTextCentered(*_resolution_help_text, _row_help_base_rect);

   if (!_video_modes.empty())
   {
      const auto& mode = _video_modes[resolution_index];
      _resolution_text->setString(std::format("{}x{}", mode[0], mode[1]));
   }

   // display mode row
   _displaymode_label->setString(sftr("Display Mode"));
   _displaymode_label->setFillColor(display_mode_selected ? color_label_selected : color_label_normal);
   placeTextLeft(*_displaymode_label, rowRect(_row_label_base_rect, 1));

   _displaymode_help_text->setString(sftr("Change the display render mode of the game"));

   placeTextCentered(*_displaymode_help_text, _row_help_base_rect);

   const sf::String display_mode_strings[] = {sftr("Windowed"), sftr("Borderless"), sftr("Fullscreen")};
   _displaymode_value_text->setString(display_mode_strings[display_mode_value_index]);
   placeTextLeft(*_displaymode_value_text, rowRect(_row_value_base_rect, 0));

   // vsync row
   _vsync_label->setString(sftr("V-Sync"));
   _vsync_label->setFillColor(vsync_selected ? color_label_selected : color_label_normal);
   placeTextLeft(*_vsync_label, rowRect(_row_label_base_rect, 2));

   _vsync_help_text->setString(sftr("Adjust the Vertical Synchronization"));

   placeTextCentered(*_vsync_help_text, _row_help_base_rect);

   _vsync_value_text->setString(vsync_enabled ? sftr("On") : sftr("Off"));
   placeTextLeft(*_vsync_value_text, rowRect(_row_value_base_rect, 1));

   // brightness row
   _brightness_label->setString(sftr("Brightness"));
   _brightness_label->setFillColor(brightness_selected ? color_label_selected : color_label_normal);
   placeTextLeft(*_brightness_label, rowRect(_row_label_base_rect, 3));

   _brightness_help_text->setString(sftr("Adjust the screen brightness"));

   placeTextCentered(*_brightness_help_text, _row_help_base_rect);

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
