#pragma once

#include "menuscreen.h"

#include <array>
#include <functional>

/// \brief video settings screen for resolution, display mode, vsync, and brightness.
class MenuScreenVideo : public MenuScreen
{
public:
   /// \brief identifies configurable rows in the video settings screen.
   enum class Selection
   {
      Resolution = 0,
      DisplayMode = 1,
      VSync = 2,
      Brightness = 3,
      Count = 4
   };

   using FullscreenCallback = std::function<void(void)>;
   using VSyncCallback = std::function<void(void)>;
   using ResolutionCallback = std::function<void(int32_t, int32_t)>;

   /// \brief initializes the video settings screen and supported resolution list.
   MenuScreenVideo();

   /// \brief routes navigation, adjustment, and cancel keys to video settings actions.
   /// \param key key that was pressed.
   void keyboardKeyPressed(sf::Keyboard::Key key) override;

   /// \brief caches brightness indicator layers after PSD loading.
   void loadingFinished() override;

   /// \brief updates highlights, prompts, and value indicators for all video settings.
   void updateLayers();

   /// \brief moves selection to the previous video row with wrap-around.
   void up();

   /// \brief moves selection to the next video row with wrap-around.
   void down();

   /// \brief applies a row-specific setting change using the provided step direction.
   /// \param step signed step used for cycling options and brightness adjustments.
   void select(int32_t step);

   /// \brief returns to the options menu.
   void back();

   /// \brief installs the callback invoked when display mode toggles.
   /// \param callback function called after display mode changes.
   void setFullscreenCallback(FullscreenCallback callback);

   /// \brief installs the callback invoked when resolution changes.
   /// \param callback function called with new width and height.
   void setResolutionCallback(ResolutionCallback callback);

   /// \brief installs the callback invoked when vsync toggles.
   /// \param callback function called after vsync state changes.
   void setVSyncCallback(VSyncCallback callback);

   Selection _selection = Selection::Resolution;

private:
   FullscreenCallback _fullscreen_callback;
   ResolutionCallback _resolution_callback;
   VSyncCallback _vsync_callback;
   std::vector<std::array<int32_t, 2>> _video_modes;
   std::vector<std::shared_ptr<Layer>> _brightness_value_layers;
};
