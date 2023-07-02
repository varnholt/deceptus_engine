#include "callbackmap.h"


CallbackMap& CallbackMap::getInstance()
{
   static CallbackMap __instance;
   return __instance;
}


void CallbackMap::addCallback(int32_t cb_id, const Callback& cb)
{
   _map[cb_id].push_back(cb);
}


void CallbackMap::call(int32_t cb_id)
{
   for (const auto& cb : _map[cb_id])
   {
      cb();
   }
}
