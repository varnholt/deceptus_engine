#pragma once

#include <array>
#include <cstdint>
#include <functional>


struct CallbackMap
{
   using Callback = std::function<void(void)>;

   static CallbackMap& getInstance();

   enum class CallbackType {
      EndGame,
      Count
   };

   void addCallback(CallbackType, Callback cb);
   void call(CallbackType);


private:

   std::array<std::vector<Callback>, static_cast<uint32_t>(CallbackType::Count)> mMap;

   CallbackMap() = default;
   static CallbackMap sInstance;
};

