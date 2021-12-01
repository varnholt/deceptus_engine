#pragma once

#include <cstdint>
#include <functional>
#include <map>


struct CallbackMap
{
   using Callback = std::function<void(void)>;

   static CallbackMap& getInstance();

   void addCallback(int32_t id, const Callback& cb);
   void call(int32_t id);


private:

   std::map<int32_t, std::vector<Callback>> _map;
   CallbackMap() = default;
};

