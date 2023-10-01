#pragma once

#include "menuscreen.h"

#include <array>
#include <functional>

class MenuScreenVideo : public MenuScreen
{
public:
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

   MenuScreenVideo();
   void keyboardKeyPressed(sf::Keyboard::Key key) override;

   void loadingFinished() override;
   void updateLayers();

   void up();
   void down();
   void select(int32_t step);
   void back();

   void setFullscreenCallback(FullscreenCallback callback);
   void setResolutionCallback(ResolutionCallback callback);
   void setVSyncCallback(VSyncCallback callback);

   Selection _selection = Selection::Resolution;

private:
   FullscreenCallback _fullscreen_callback;
   ResolutionCallback _resolution_callback;
   VSyncCallback _vsync_callback;
   std::vector<std::array<int32_t, 2>> _video_modes;
   std::vector<std::shared_ptr<Layer>> _brightness_value_layers;
};
