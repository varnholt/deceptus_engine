#include "timer.h"

#include <algorithm>

std::vector<std::unique_ptr<Timer>> Timer::__timers;
std::mutex Timer::__mutex;

void Timer::update(Scope scope)
{
   auto now = std::chrono::high_resolution_clock::now();

   std::lock_guard<std::mutex> guard(__mutex);

   // it might be better to
   // 1) lock
   // 2) find the timer
   // 3) unlock
   // 4) call the update function
   // 5) lock/cleanup/unlock
   //
   // background: if the callback starts another timer, the mutex is locked :/

   __timers.erase(
      std::remove_if(
         __timers.begin(),
         __timers.end(),
         [now, scope](auto& timer) -> bool
         {
            if (timer->_scope != scope)
            {
               return false;
            }

            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - timer->_start_time);
            auto delta = elapsed - timer->_interval;

            if (delta > std::chrono::milliseconds(0))
            {
               timer->_callback();

               if (timer->_type == Type::Singleshot)
               {
                  return true;
               }
               else
               {
                  timer->_start_time = now + delta;
                  return false;
               }
            }

            return false;
         }
      ),
      __timers.end()
   );
}

void Timer::add(
   std::chrono::milliseconds interval,
   std::function<void()> callback,
   Type type,
   Scope scope,
   const std::shared_ptr<void>& data,
   const std::shared_ptr<void>& caller
)
{
   std::unique_ptr<Timer> timer = std::make_unique<Timer>();
   timer->_interval = interval;
   timer->_type = type;
   timer->_scope = scope;
   timer->_start_time = std::chrono::high_resolution_clock::now();
   timer->_callback = callback;
   timer->_data = data;
   timer->_caller = caller;

   // don't start another timer from the timed function in your lua code :)
   std::lock_guard<std::mutex> guard(__mutex);
   __timers.push_back(std::move(timer));
}

void Timer::removeByCaller(const std::shared_ptr<void>& caller)
{
   std::lock_guard<std::mutex> guard(__mutex);

   __timers.erase(
      std::remove_if(__timers.begin(), __timers.end(), [caller](auto& timer) -> bool { return timer->_caller == caller; }), __timers.end()
   );
}
