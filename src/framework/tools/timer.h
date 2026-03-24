#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

///
/// \brief Manages process-wide timed callbacks for game and global update scopes.
///
class Timer
{
public:
   ///
   /// \brief Controls whether a timer fires once or repeatedly.
   ///
   enum class Type
   {
      Singleshot,
      Repeated
   };

   ///
   /// \brief Selects which update pass is allowed to advance a timer.
   ///
   enum class Scope
   {
      UpdateAlways,
      UpdateIngame
   };

   Timer() = default;
   ///
   /// \brief Destroys the timer.
   ///
   ~Timer() = default;

   ///
   /// \brief Advances all timers matching `scope` and runs due callbacks.
   /// \param scope Update scope currently being processed.
   ///
   static void update(Scope scope);
   ///
   /// \brief Registers a new timer callback.
   /// \param interval Delay between callback invocations.
   /// \param callback Function to invoke when the timer expires.
   /// \param type Single-shot or repeated behavior.
   /// \param scope Update scope required for this timer to advance.
   /// \param data Optional attached user data.
   /// \param caller Optional owner token for grouped removal.
   ///
   static void add(
      std::chrono::milliseconds interval,
      std::function<void()> callback,
      Type type = Type::Singleshot,
      Scope scope = Scope::UpdateAlways,
      const std::shared_ptr<void>& data = nullptr,
      const std::shared_ptr<void>& caller = nullptr
   );

   ///
   /// \brief Removes all timers associated with `caller`.
   /// \param caller Owner token used when creating timers.
   ///
   static void removeByCaller(const std::shared_ptr<void>& caller);

   std::chrono::milliseconds _interval;
   Type _type = Type::Singleshot;
   Scope _scope = Scope::UpdateAlways;

   std::function<void()> _callback = nullptr;
   std::chrono::high_resolution_clock::time_point _start_time;
   std::shared_ptr<void> _data;
   std::shared_ptr<void> _caller;

private:
   static std::vector<std::unique_ptr<Timer>> __timers;
   static std::mutex __mutex;
};
