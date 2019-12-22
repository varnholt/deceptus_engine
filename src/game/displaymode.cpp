#include "displaymode.h"


//-----------------------------------------------------------------------------
DisplayMode DisplayMode::sInstance;


//-----------------------------------------------------------------------------
void DisplayMode::toggle(Display mode)
{
  if (mMode & mode)
  {
     mMode &= ~ mode;
  }
  else
  {
     mMode |= mode;
  }
}


//-----------------------------------------------------------------------------
void DisplayMode::sync()
{
   for (auto& mode : mQueuedSet)
   {
      mMode |= mode;
   }

   for (auto& mode : mQueuedUnset)
   {
      mMode &= ~ mode;
   }

   for (auto& mode : mQueuedToggle)
   {
      toggle(mode);
   }

   mQueuedSet.clear();
   mQueuedUnset.clear();
   mQueuedToggle.clear();
}


//-----------------------------------------------------------------------------
void DisplayMode::enqueueSet(Display mode)
{
   mQueuedSet.push_back(mode);
}


//-----------------------------------------------------------------------------
void DisplayMode::enqueueUnset(Display mode)
{
   mQueuedUnset.push_back(mode);
}


void DisplayMode::enqueueToggle(Display mode)
{
   mQueuedToggle.push_back(mode);
}


//-----------------------------------------------------------------------------
int32_t DisplayMode::get()
{
  return mMode;
}


//-----------------------------------------------------------------------------
bool DisplayMode::isSet(Display mode)
{
   return mMode & mode;
}


//-----------------------------------------------------------------------------
DisplayMode& DisplayMode::getInstance()
{
  return sInstance;
}
