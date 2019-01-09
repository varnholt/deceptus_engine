#ifndef TIMER_H
#define TIMER_H

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
      Repetetive
   };

   std::chrono::milliseconds mInterval;
   Type mType = Type::Singleshot;
   std::function<void()> mCallback = nullptr;
   std::chrono::high_resolution_clock::time_point mStartTime;
   std::shared_ptr<void> mData;


private:
   static std::vector<std::unique_ptr<Timer>> mTimers;
   static std::mutex mMutex;


public:
   Timer() = default;
   ~Timer() = default;

   static void update();
   static void add(
      std::chrono::milliseconds interval,
      std::function<void()>,
      Type type = Type::Singleshot,
      std::shared_ptr<void> data = nullptr
   );
};

#endif // TIMER_H
