#include "menuscreenvideo.h"

#include "menu.h"

#include "game/gameconfiguration.h"


static const auto STEP_SIZE = 10;



MenuScreenVideo::MenuScreenVideo()
{
   setFilename("data/menus/video.psd");

   mVideoModes = { {1024, 576}, {1280, 720}, {1366, 864}, {1536, 864}, {1600, 900}, {1920, 1080}, {3840, 2160} };
}



void MenuScreenVideo::up()
{
   auto next = static_cast<int32_t>(mSelection);
   next--;
   if (next < 0)
   {
      next = static_cast<int32_t>(Selection::Count) - 1;
   }

   mSelection = static_cast<Selection>(next);
   updateLayers();
}


void MenuScreenVideo::down()
{
   auto next = static_cast<int32_t>(mSelection);
   next++;
   if (next == static_cast<int32_t>(Selection::Count))
   {
      next = 0;
   }

   mSelection = static_cast<Selection>(next);
   updateLayers();
}


void MenuScreenVideo::select(int32_t step)
{
   switch (mSelection)
   {
       case Selection::DisplayMode:
       {
          mFullscreenCallback();
          break;
       }

       case Selection::Resolution:
       {
          auto next = [this, step]() -> std::array<int32_t, 2> {
              auto it = std::find_if(std::begin(mVideoModes), std::end(mVideoModes), [](const std::array<int32_t, 2> arr){
                  return
                         arr[0] == GameConfiguration::getInstance()._video_mode_width
                      && arr[1] == GameConfiguration::getInstance()._video_mode_height;
              });

              auto index = it - mVideoModes.begin();
              if (step < 0)
                  index--;
              else
                  index++;

              if (index < 0)
              {
                  index = mVideoModes.size() - 1;
              }
              else if (index > static_cast<int32_t>(mVideoModes.size() - 1))
              {
                  index = 0;
              }
              return mVideoModes[index];
          }();

          mResolutionCallback(next[0], next[1]);
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
         mVsyncCallback();
         break;
      }

      case Selection::Monitor:
      case Selection::Count:
      {
         break;
      }
   }

   GameConfiguration::getInstance().serializeToFile();
   updateLayers();
}


void MenuScreenVideo::back()
{
    Menu::getInstance()->show(Menu::MenuType::Options);
}


void MenuScreenVideo::setFullscreenCallback(MenuScreenVideo::FullscreenCallback callback)
{
    mFullscreenCallback = callback;
}


void MenuScreenVideo::setResolutionCallback(MenuScreenVideo::ResolutionCallback callback)
{
   mResolutionCallback = callback;
}


void MenuScreenVideo::setVSyncCallback(VSyncCallback callback)
{
   mVsyncCallback = callback;
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
   updateLayers();
}


void MenuScreenVideo::updateLayers()
{
   auto resolution = mSelection == Selection::Resolution;
   auto displayMode = mSelection == Selection::DisplayMode;
   auto vsync = mSelection == Selection::VSync;
   auto brightness = mSelection == Selection::Brightness;

   auto resolutionSelection = 0u;
   auto displayModeSelection = 0;

   auto fullscreen = GameConfiguration::getInstance()._fullscreen;
   if (fullscreen)
   {
       displayModeSelection = 2;
   }

   auto resolution_width = GameConfiguration::getInstance()._video_mode_width;
   auto resolution_height = GameConfiguration::getInstance()._video_mode_height;

   for (auto index = 0u; index < mVideoModes.size(); index++)
   {
      if (
            mVideoModes[index][0] == resolution_width
         && mVideoModes[index][1] == resolution_height
      )
      {
         resolutionSelection = index;
         break;
      }
   }

   const auto brightnessValue = GameConfiguration::getInstance()._brightness;
   const auto vsyncSelection = (GameConfiguration::getInstance()._vsync_enabled) ? 1 : 0;

   mLayers["defaults_xbox_0"]->_visible = isControllerUsed();
   mLayers["defaults_xbox_1"]->_visible = false;
   mLayers["back_xbox_0"]->_visible = isControllerUsed();
   mLayers["back_xbox_1"]->_visible = false;

   mLayers["defaults_pc_0"]->_visible = !isControllerUsed();
   mLayers["defaults_pc_1"]->_visible = false;
   mLayers["back_pc_0"]->_visible = !isControllerUsed();
   mLayers["back_pc_1"]->_visible = false;

   mLayers["resolution_text_0"]->_visible = !resolution;
   mLayers["resolution_text_1"]->_visible = resolution;
   mLayers["resolution_help"]->_visible = resolution;
   mLayers["resolution_highlight"]->_visible = resolution;
   mLayers["resolution_arrows"]->_visible = resolution;
   mLayers["resolution_value_1024x576"]->_visible = resolutionSelection == 0;
   mLayers["resolution_value_1280x720"]->_visible = resolutionSelection == 1;
   mLayers["resolution_value_1366x768"]->_visible = resolutionSelection == 2;
   mLayers["resolution_value_1536x864"]->_visible = resolutionSelection == 3;
   mLayers["resolution_value_1600x900"]->_visible = resolutionSelection == 4;
   mLayers["resolution_value_1920x1080"]->_visible = resolutionSelection == 5;
   mLayers["resolution_value_3840x2160"]->_visible = resolutionSelection == 6;

   mLayers["brightness_text_0"]->_visible = !brightness;
   mLayers["brightness_text_1"]->_visible = brightness;
   mLayers["brightness_body_0"]->_visible = !brightness;
   mLayers["brightness_body_1"]->_visible = brightness;
   mLayers["brightness_highlight"]->_visible = brightness;
   mLayers["brightness_help"]->_visible = brightness;
   mLayers["brightness_arrows"]->_visible = brightness;
   mLayers["brightness_h_0"]->_visible = !brightness;
   mLayers["brightness_h_1"]->_visible = brightness;
   mLayers["brightness_value"]->_visible = true;
   mLayers["brightness_h_0"]->_sprite->setOrigin(50 - (brightnessValue * 100.0f), 0);
   mLayers["brightness_h_1"]->_sprite->setOrigin(50 - (brightnessValue * 100.0f), 0);

   mLayers["displayMode_text_0"]->_visible = !displayMode;
   mLayers["displayMode_text_1"]->_visible = displayMode;
   mLayers["displayMode_highlight"]->_visible = displayMode;
   mLayers["displayMode_help"]->_visible = displayMode;
   mLayers["displayMode_arrows"]->_visible = displayMode;
   mLayers["displayMode_value_windowed"]->_visible = displayModeSelection == 0;
   mLayers["displayMode_value_borderless"]->_visible = displayModeSelection == 1;
   mLayers["displayMode_value_fullscreen"]->_visible = displayModeSelection == 2;

   mLayers["vSync_text_0"]->_visible = !vsync;
   mLayers["vSync_text_1"]->_visible = vsync;
   mLayers["vSync_highlight"]->_visible = vsync;
   mLayers["vSync_help"]->_visible = vsync;
   mLayers["vSync_arrows"]->_visible = vsync;
   mLayers["vSync_value_0"]->_visible = vsyncSelection == 0;
   mLayers["vSync_value_1"]->_visible = vsyncSelection == 1;

}


/*
data/menus/video.psd

   bg_temp

   video-window-bg
   video_window-main

   header
*/

