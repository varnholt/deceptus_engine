#include "menus/menuscreenvideo.h"

#include "game/gameconfiguration.h"
#include "menus/menu.h"
#include "menus/menuaudio.h"

#include <cmath>
#include <format>

static const auto STEP_SIZE = 10;

MenuScreenVideo::MenuScreenVideo()
{
   setFilename("data/menus/video.psd");

   _video_modes = {{1024, 576}, {1280, 720}, {1366, 864}, {1536, 864}, {1600, 900}, {1920, 1080}, {3840, 2160}};
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
         auto next = [this, step]() -> std::array<int32_t, 2>
         {
            auto it = std::find_if(
               std::begin(_video_modes),
               std::end(_video_modes),
               [](const std::array<int32_t, 2> arr) {
                  return arr[0] == GameConfiguration::getInstance()._video_mode_width &&
                         arr[1] == GameConfiguration::getInstance()._video_mode_height;
               }
            );

            auto index = it - _video_modes.begin();
            if (step < 0)
               index--;
            else
               index++;

            if (index < 0)
            {
               index = _video_modes.size() - 1;
            }
            else if (index > static_cast<int32_t>(_video_modes.size() - 1))
            {
               index = 0;
            }
            return _video_modes[index];
         }();

         _resolution_callback(next[0], next[1]);
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
   if (key == sf::Keyboard::Up)
   {
      up();
   }

   else if (key == sf::Keyboard::Down)
   {
      down();
   }

   else if (key == sf::Keyboard::Left)
   {
      select(-STEP_SIZE);
   }

   else if (key == sf::Keyboard::Right)
   {
      select(STEP_SIZE);
   }

   else if (key == sf::Keyboard::Escape)
   {
      back();
   }
}

void MenuScreenVideo::loadingFinished()
{
   for (auto i = 0; i < 11; i++)
   {
      const auto brightness_value_layer_name = std::format("brightness_value_{}", i);
      _brightness_value_layers.push_back(_layers[brightness_value_layer_name]);
   }

   updateLayers();
}

void MenuScreenVideo::updateLayers()
{
   auto resolution = _selection == Selection::Resolution;
   auto displayMode = _selection == Selection::DisplayMode;
   auto vsync = _selection == Selection::VSync;
   auto brightness = _selection == Selection::Brightness;

   auto resolution_selection = 0u;
   auto display_mode_selection = 0;

   auto fullscreen = GameConfiguration::getInstance()._fullscreen;
   if (fullscreen)
   {
      display_mode_selection = 2;
   }

   auto resolution_width = GameConfiguration::getInstance()._video_mode_width;
   auto resolution_height = GameConfiguration::getInstance()._video_mode_height;

   for (auto index = 0u; index < _video_modes.size(); index++)
   {
      if (_video_modes[index][0] == resolution_width && _video_modes[index][1] == resolution_height)
      {
         resolution_selection = index;
         break;
      }
   }

   const auto brightness_value = GameConfiguration::getInstance()._brightness;
   const auto vsync_selection = (GameConfiguration::getInstance()._vsync_enabled) ? 1 : 0;

   _layers["defaults_xbox_0"]->_visible = isControllerUsed();
   _layers["defaults_xbox_1"]->_visible = false;
   _layers["back_xbox_0"]->_visible = isControllerUsed();
   _layers["back_xbox_1"]->_visible = false;

   _layers["defaults_pc_0"]->_visible = !isControllerUsed();
   _layers["defaults_pc_1"]->_visible = false;
   _layers["back_pc_0"]->_visible = !isControllerUsed();
   _layers["back_pc_1"]->_visible = false;

   _layers["resolution_text_0"]->_visible = !resolution;
   _layers["resolution_text_1"]->_visible = resolution;
   _layers["resolution_help"]->_visible = resolution;
   _layers["resolution_highlight"]->_visible = resolution;
   _layers["resolution_arrows"]->_visible = resolution;
   _layers["resolution_value_1024x576"]->_visible = resolution_selection == 0;
   _layers["resolution_value_1280x720"]->_visible = resolution_selection == 1;
   _layers["resolution_value_1366x768"]->_visible = resolution_selection == 2;
   _layers["resolution_value_1536x864"]->_visible = resolution_selection == 3;
   _layers["resolution_value_1600x900"]->_visible = resolution_selection == 4;
   _layers["resolution_value_1920x1080"]->_visible = resolution_selection == 5;
   _layers["resolution_value_3840x2160"]->_visible = resolution_selection == 6;

   _layers["brightness_text_0"]->_visible = !brightness;
   _layers["brightness_text_1"]->_visible = brightness;
   _layers["brightness_body_0"]->_visible = !brightness;
   _layers["brightness_body_1"]->_visible = brightness;
   _layers["brightness_highlight"]->_visible = brightness;
   _layers["brightness_help"]->_visible = brightness;
   _layers["brightness_arrows"]->_visible = brightness;
   _layers["brightness_h_0"]->_visible = !brightness;
   _layers["brightness_h_1"]->_visible = brightness;

   _layers["brightness_h_0"]->_sprite->setOrigin(50 - (brightness_value * 100.0f), 0);
   _layers["brightness_h_1"]->_sprite->setOrigin(50 - (brightness_value * 100.0f), 0);

   _layers["displayMode_text_0"]->_visible = !displayMode;
   _layers["displayMode_text_1"]->_visible = displayMode;
   _layers["displayMode_highlight"]->_visible = displayMode;
   _layers["displayMode_help"]->_visible = displayMode;
   _layers["displayMode_arrows"]->_visible = displayMode;
   _layers["displayMode_value_windowed"]->_visible = display_mode_selection == 0;
   _layers["displayMode_value_borderless"]->_visible = display_mode_selection == 1;
   _layers["displayMode_value_fullscreen"]->_visible = display_mode_selection == 2;

   _layers["vSync_text_0"]->_visible = !vsync;
   _layers["vSync_text_1"]->_visible = vsync;
   _layers["vSync_highlight"]->_visible = vsync;
   _layers["vSync_help"]->_visible = vsync;
   _layers["vSync_arrows"]->_visible = vsync;
   _layers["vSync_value_0"]->_visible = vsync_selection == 0;
   _layers["vSync_value_1"]->_visible = vsync_selection == 1;

   const auto brightness_index = static_cast<int32_t>(std::ceil((brightness_value * 10.0f) - 0.1f));
   for (auto i = 0; i < 11; i++)
   {
      _brightness_value_layers[i]->_visible = (i == brightness_index);
   }
}

/*
data/menus/video.psd

   bg_temp

   video-window-bg
   video_window-main

   header
*/
