#pragma once

#include <cstdint>
#include <functional>
#include <vector>
#include "constants.h"

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

   using QueuedFunction = std::function<void(void)>;

   std::vector<QueuedFunction> _queue;
};

