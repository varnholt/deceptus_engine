#pragma once

#include <array>
#include <cstdint>
#include <functional>


struct CallbackMap
{
   using Callback = std::function<void(void)>;

   static CallbackMap& getInstance();

   // it might make more sense to remove game related stuff here and use a simple uint32_t _id
   enum class CallbackType {
      EndGame,
      Count
   };

   void addCallback(CallbackType, Callback cb);
   void call(CallbackType);


private:

   std::array<std::vector<Callback>, static_cast<uint32_t>(CallbackType::Count)> mMap;

   CallbackMap() = default;
};

