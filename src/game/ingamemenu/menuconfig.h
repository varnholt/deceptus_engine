#ifndef MENUCONFIG_H
#define MENUCONFIG_H

#include <chrono>

/// \brief stores configurable show and hide durations for in-game menu animations.
struct MenuConfig
{
   /// \brief loads menu timing values from data/config/menus.json.
   MenuConfig();

   using FloatSeconds = std::chrono::duration<float>;

   FloatSeconds _duration_show;
   FloatSeconds _duration_hide;
};

#endif  // MENUCONFIG_H
