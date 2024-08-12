#ifndef MENUCONFIG_H
#define MENUCONFIG_H

#include <chrono>

struct MenuConfig
{
   MenuConfig();

   using FloatSeconds = std::chrono::duration<float>;

   FloatSeconds _duration_show;
   FloatSeconds _duration_hide;
};

#endif  // MENUCONFIG_H
