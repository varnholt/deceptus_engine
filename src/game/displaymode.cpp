#include "displaymode.h"

void DisplayMode::toggle(Display mode)
{
   if (_mode & static_cast<int32_t>(mode))
   {
      _mode &= ~static_cast<int32_t>(mode);
   }
   else
   {
      _mode |= static_cast<int32_t>(mode);
   }
}

void DisplayMode::sync()
{
   for (auto& change : _queue)
   {
      change();
   }

   _queue.clear();
}

void DisplayMode::enqueueSet(Display mode)
{
   _queue.push_back([this, mode]() { _mode |= static_cast<int32_t>(mode); });
}

void DisplayMode::enqueueUnset(Display mode)
{
   _queue.push_back([this, mode]() { _mode &= ~static_cast<int32_t>(mode); });
}

void DisplayMode::enqueueToggle(Display mode)
{
   _queue.push_back([this, mode]() { toggle(mode); });
}

int32_t DisplayMode::get() const
{
   return _mode;
}

bool DisplayMode::isSet(Display mode) const
{
   return _mode & static_cast<int32_t>(mode);
}

DisplayMode& DisplayMode::getInstance()
{
   static DisplayMode __instance;
   return __instance;
}
