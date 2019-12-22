#pragma once

#include "constants.h"
#include <cstdint>
#include <vector>

class DisplayMode
{

private:

   int32_t mMode = Display::DisplayGame;

   std::vector<Display> mQueuedSet;
   std::vector<Display> mQueuedUnset;
   std::vector<Display> mQueuedToggle;

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
};

