#include "menuscreenvideo.h"

#include "menu.h"

#include "game/gameconfiguration.h"


static const auto STEP_SIZE = 10;



MenuScreenVideo::MenuScreenVideo()
{
   setFilename("data/menus/video.psd");

   mVideoModes = { {1024, 576}, {1280, 720}, {1366, 864}, {1536, 864}, {1600, 900}, {1920, 1080} };
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
            updateLayers();
            break;
        }

        case Selection::Resolution:
        {
            auto next = [this, step]() -> std::array<int32_t, 2> {
                auto it = std::find_if(std::begin(mVideoModes), std::end(mVideoModes), [](const std::array<int32_t, 2> arr){
                    return
                           arr[0] == GameConfiguration::getInstance().mVideoModeWidth
                        && arr[1] == GameConfiguration::getInstance().mVideoModeHeight;
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
           float brightness = GameConfiguration::getInstance().mBrightness;
           brightness += (0.01f * step);

           if (brightness < 0.0f)
           {
              brightness = 0.0f;
           }
           else if (brightness > 1.0f)
           {
              brightness = 1.0f;
           }

           GameConfiguration::getInstance().mBrightness = brightness;
           break;
        }

        case Selection::VSync:
        {
           GameConfiguration::getInstance().mVSync = !GameConfiguration::getInstance().mVSync;
           mVsyncCallback();
           break;
        }

        default:
            break;
    }

    GameConfiguration::getInstance().serializeToFile();
    updateLayers();
}


void MenuScreenVideo::back()
{
    Menu::getInstance().show(Menu::MenuType::Options);
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
   auto monitor = mSelection == Selection::Monitor;
   auto resolution = mSelection == Selection::Resolution;
   auto displayMode = mSelection == Selection::DisplayMode;
   auto vsync = mSelection == Selection::VSync;
   auto brightness = mSelection == Selection::Brightness;

   auto monitorSelection = 0;
   auto resolutionSelection = 0u;
   auto displayModeSelection = 0;

   auto fullscreen = GameConfiguration::getInstance().mFullscreen;
   if (fullscreen)
   {
       displayModeSelection = 2;
   }

   auto resolution_width = GameConfiguration::getInstance().mVideoModeWidth;
   auto resolution_height = GameConfiguration::getInstance().mVideoModeHeight;

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

   const auto brightnessValue = GameConfiguration::getInstance().mBrightness;
   const auto vsyncSelection = (GameConfiguration::getInstance().mVSync) ? 0 : 1;

   mLayers["defaults_xbox_0"]->mVisible = false;
   mLayers["defaults_xbox_1"]->mVisible = false;
   mLayers["back_xbox_0"]->mVisible = false;
   mLayers["back_xbox_1"]->mVisible = false;

   mLayers["defaults_pc_0"]->mVisible = true;
   mLayers["defaults_pc_1"]->mVisible = false;
   mLayers["back_pc_0"]->mVisible = true;
   mLayers["back_pc_1"]->mVisible = false;

   mLayers["monitor_text_0"]->mVisible = !monitor;
   mLayers["monitor_text_1"]->mVisible = monitor;
   mLayers["monitor_help"]->mVisible = monitor;
   mLayers["monitor_highlight"]->mVisible = monitor;
   mLayers["monitor_arrows"]->mVisible = monitor;
   mLayers["monitor_value_1"]->mVisible = monitorSelection == 0;
   mLayers["monitor_value_2"]->mVisible = monitorSelection == 1;
   mLayers["monitor_value_3"]->mVisible = monitorSelection == 2;
   mLayers["monitor_value_4"]->mVisible = monitorSelection == 3;

   mLayers["resolution_text_0"]->mVisible = !resolution;
   mLayers["resolution_text_1"]->mVisible = resolution;
   mLayers["resolution_help"]->mVisible = resolution;
   mLayers["resolution_highlight"]->mVisible = resolution;
   mLayers["resolution_arrows"]->mVisible = resolution;
   mLayers["resolution_value_1024x576"]->mVisible = resolutionSelection == 0;
   mLayers["resolution_value_1280x720"]->mVisible = resolutionSelection == 1;
   mLayers["resolution_value_1366x768"]->mVisible = resolutionSelection == 2;
   mLayers["resolution_value_1536x864"]->mVisible = resolutionSelection == 3;
   mLayers["resolution_value_1600x900"]->mVisible = resolutionSelection == 4;
   mLayers["resolution_value_1920x1080"]->mVisible = resolutionSelection == 5;

   mLayers["brightness_text_0"]->mVisible = !brightness;
   mLayers["brightness_text_1"]->mVisible = brightness;
   mLayers["brightness_body_0"]->mVisible = !brightness;
   mLayers["brightness_body_1"]->mVisible = brightness;
   mLayers["brightness_highlight"]->mVisible = brightness;
   mLayers["brightness_help"]->mVisible = brightness;
   mLayers["brightness_arrows"]->mVisible = brightness;
   mLayers["brightness_h"]->mVisible = true;
   mLayers["brightness_value"]->mVisible = true;
   mLayers["brightness_h"]->mSprite->setOrigin(50 - (brightnessValue * 100.0f), 0);

   mLayers["displayMode_text_0"]->mVisible = !displayMode;
   mLayers["displayMode_text_1"]->mVisible = displayMode;
   mLayers["displayMode_highlight"]->mVisible = displayMode;
   mLayers["displayMode_help"]->mVisible = displayMode;
   mLayers["displayMode_arrows"]->mVisible = displayMode;
   mLayers["displayMode_value_windowed"]->mVisible = displayModeSelection == 0;
   mLayers["displayMode_value_borderless"]->mVisible = displayModeSelection == 1;
   mLayers["displayMode_value_fullscreen"]->mVisible = displayModeSelection == 2;

   mLayers["vSync_text_0"]->mVisible = !vsync;
   mLayers["vSync_text_1"]->mVisible = vsync;
   mLayers["vSync_highlight"]->mVisible = vsync;
   mLayers["vSync_help"]->mVisible = vsync;
   mLayers["vSync_arrows"]->mVisible = vsync;
   mLayers["vSync_value_0"]->mVisible = vsyncSelection == 0;
   mLayers["vSync_value_1"]->mVisible = vsyncSelection == 1;

}


/*
data/menus/video.psd

   bg_temp

   video-window-bg
   video_window-main

   header
*/

