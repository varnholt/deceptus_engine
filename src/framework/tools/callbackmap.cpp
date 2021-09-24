#include "callbackmap.h"


CallbackMap& CallbackMap::getInstance()
{
   static CallbackMap __instance;
   return __instance;
}


void CallbackMap::addCallback(CallbackMap::CallbackType cbType, CallbackMap::Callback cb)
{
   _map[static_cast<uint32_t>(cbType)].push_back(cb);
}


void CallbackMap::call(CallbackMap::CallbackType cbType)
{
   for (auto& cb : _map[static_cast<uint32_t>(cbType)])
   {
      cb();
   }
}
