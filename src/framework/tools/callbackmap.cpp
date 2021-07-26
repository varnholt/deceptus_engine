#include "callbackmap.h"


CallbackMap& CallbackMap::getInstance()
{
   static CallbackMap sInstance;
   return sInstance;
}


void CallbackMap::addCallback(CallbackMap::CallbackType cbType, CallbackMap::Callback cb)
{
   mMap[static_cast<uint32_t>(cbType)].push_back(cb);
}


void CallbackMap::call(CallbackMap::CallbackType cbType)
{
   for (auto& cb : mMap[static_cast<uint32_t>(cbType)])
   {
      cb();
   }
}
