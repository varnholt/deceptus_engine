#pragma once

#include "constants.h"
#include <cstdint>
#include <vector>

class DisplayMode
{
public:

   DisplayMode() = default;

   static DisplayMode& getInstance();
   static DisplayMode sInstance;


   void sync();
   void enqueueSet(Display mode);
   void enqueueUnset(Display mode);
   void enqueueToggle(Display mode);

   int32_t get();
   bool isSet(Display mode);

private:

   void toggle(Display mode);

   int32_t _mode = Display::DisplayGame;

   std::vector<Display> _queued_set;
   std::vector<Display> _queued_unset;
   std::vector<Display> _queued_toggle;

};

