#include "timer.h"

#include <algorithm>


std::vector<std::unique_ptr<Timer>> Timer::mTimers;
std::mutex Timer::mMutex;


void Timer::update()
{
   auto now = std::chrono::high_resolution_clock::now();

   std::lock_guard<std::mutex> guard(mMutex);

   // it might be better to
   // 1) lock
   // 2) find the timer
   // 3) unlock
   // 4) call the update function
   // 5) lock/cleanup/unlock
   //
   // background: if the callback starts another timer, the mutex is locked :/

   mTimers.erase(
      std::remove_if(
         mTimers.begin(),
         mTimers.end(),
         [now](auto& timer) -> bool
         {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - timer->mStartTime);
            auto delta = elapsed - timer->mInterval;

            if (delta > std::chrono::milliseconds(0))
            {
               timer->mCallback();

               if (timer->mType == Type::Singleshot)
               {
                  return true;
               }
               else
               {
                  timer->mStartTime = now + delta;
                  return false;
               }
            }

            return false;
         }
      ),
      mTimers.end()
   );
}


void Timer::add(
   std::chrono::milliseconds interval,
   std::function<void ()> callback,
   Type type,
   std::shared_ptr<void> data
)
{
   std::unique_ptr<Timer> timer = std::make_unique<Timer>();
   timer->mInterval = interval;
   timer->mType = type;
   timer->mStartTime = std::chrono::high_resolution_clock::now();
   timer->mCallback = callback;
   timer->mData = data;

   // don't start another timer from the timed function in your lua code :)
   std::lock_guard<std::mutex> guard(mMutex);
   mTimers.push_back(std::move(timer));
}

