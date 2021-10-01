#include "displaymode.h"


//-----------------------------------------------------------------------------
void DisplayMode::toggle(Display mode)
{
  if (_mode & static_cast<int32_t>(mode))
  {
     _mode &= ~ static_cast<int32_t>(mode);
  }
  else
  {
     _mode |= static_cast<int32_t>(mode);
  }
}


//-----------------------------------------------------------------------------
void DisplayMode::sync()
{
   for (auto& mode : _queued_set)
   {
      _mode |= static_cast<int32_t>(mode);
   }

   for (auto& mode : _queued_unset)
   {
      _mode &= ~ static_cast<int32_t>(mode);
   }

   for (auto& mode : _queued_toggle)
   {
      toggle(mode);
   }

   _queued_set.clear();
   _queued_unset.clear();
   _queued_toggle.clear();
}


//-----------------------------------------------------------------------------
void DisplayMode::enqueueSet(Display mode)
{
   _queued_set.push_back(mode);
}


//-----------------------------------------------------------------------------
void DisplayMode::enqueueUnset(Display mode)
{
   _queued_unset.push_back(mode);
}


void DisplayMode::enqueueToggle(Display mode)
{
   _queued_toggle.push_back(mode);
}


//-----------------------------------------------------------------------------
int32_t DisplayMode::get() const
{
  return _mode;
}


//-----------------------------------------------------------------------------
bool DisplayMode::isSet(Display mode) const
{
   return _mode & static_cast<int32_t>(mode);
}


//-----------------------------------------------------------------------------
DisplayMode& DisplayMode::getInstance()
{
   static DisplayMode __instance;
   return __instance;
}
