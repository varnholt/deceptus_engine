#pragma once

#include "constants.h"
#include <cstdint>
#include <vector>

class DisplayMode
{
public:

   DisplayMode() = default;

   static DisplayMode& getInstance();

   void sync();
   void enqueueSet(Display mode);
   void enqueueUnset(Display mode);
   void enqueueToggle(Display mode);

   int32_t get() const;
   bool isSet(Display mode) const;

private:

   void toggle(Display mode);

   int32_t _mode = static_cast<int32_t>(Display::Game);

   std::vector<Display> _queued_set;
   std::vector<Display> _queued_unset;
   std::vector<Display> _queued_toggle;

};

