#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

class Timer
{

public:

   enum class Type
   {
      Singleshot,
      Repeated
   };

   Timer() = default;
   ~Timer() = default;

   static void update();
   static void add(
      std::chrono::milliseconds interval,
      std::function<void()>,
      Type type = Type::Singleshot,
      const std::shared_ptr<void>& data = nullptr,
      const std::shared_ptr<void>& caller = nullptr
   );

   static void removeByCaller(const std::shared_ptr<void>& caller);

   std::chrono::milliseconds _interval;
   Type _type = Type::Singleshot;
   std::function<void()> _callback = nullptr;
   std::chrono::high_resolution_clock::time_point _start_time;
   std::shared_ptr<void> _data;
   std::shared_ptr<void> _caller;

private:
   static std::vector<std::unique_ptr<Timer>> __timers;
   static std::mutex __mutex;
};

