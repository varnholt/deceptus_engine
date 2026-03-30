#ifndef EVENTDISTRIBUTOR_H
#define EVENTDISTRIBUTOR_H

#include <SFML/Window/Event.hpp>
#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <vector>

/// \file eventdistributor.h
/// \brief provides a thread-safe, per-type callback registry for runtime events.

namespace EventDistributor
{

/// \brief callback type used for a specific event payload type.
/// \tparam EventT event type delivered to subscribers.
template <typename EventT>
using EventCallback = std::function<void(const EventT&)>;

/// \brief generates a unique callback identifier across all event types.
/// \return monotonically increasing callback id.
inline int32_t getNextCallbackId()
{
   static std::atomic<int32_t> current_id{0};
   return ++current_id;
}

/// \brief stores a callback id together with its handler function.
/// \tparam EventT event type associated with the callback.
template <typename EventT>
using CallbackEntry = std::pair<int32_t, EventCallback<EventT>>;

/// \brief returns the static callback list for one event type.
/// \tparam EventT event type whose callback list is requested.
/// \return mutable callback storage for the given event type.
template <typename EventT>
std::vector<CallbackEntry<EventT>>& getCallbackList()
{
   static std::vector<CallbackEntry<EventT>> callbacks;
   return callbacks;
}

/// \brief returns the mutex guarding the callback list for one event type.
/// \tparam EventT event type whose callback mutex is requested.
/// \return mutex used to synchronize register, dispatch, and unregister calls.
template <typename EventT>
std::mutex& getCallbackMutex()
{
   static std::mutex mtx;
   return mtx;
}

/// \brief dispatches an event to all callbacks registered for the same event type.
/// \tparam EventT event type being dispatched.
/// \param event event payload forwarded to every subscribed callback.
template <typename EventT>
void event(const EventT& event)
{
   auto& list = getCallbackList<EventT>();
   auto& mutex = getCallbackMutex<EventT>();
   std::lock_guard lock(mutex);

   for (auto& [id, callback] : list)
   {
      callback(event);
   }
}

/// \brief registers a callback for a specific event type.
/// \tparam EventT event type accepted by the callback.
/// \param callback function invoked when an event of type EventT is dispatched.
/// \return callback id that can be passed to unregisterEvent.
template <typename EventT>
int32_t registerEvent(EventCallback<EventT> callback)
{
   auto& list = getCallbackList<EventT>();
   auto& mutex = getCallbackMutex<EventT>();
   std::lock_guard lock(mutex);

   int32_t id = getNextCallbackId();
   list.emplace_back(id, std::move(callback));
   return id;
}

/// \brief removes a callback from an event type by callback id.
/// \tparam EventT event type registry to remove the callback from.
/// \param id callback id previously returned by registerEvent.
template <typename EventT>
void unregisterEvent(int32_t id)
{
   auto& list = getCallbackList<EventT>();
   auto& mutex = getCallbackMutex<EventT>();
   std::lock_guard lock(mutex);

   list.erase(std::remove_if(list.begin(), list.end(), [id](const CallbackEntry<EventT>& entry) { return entry.first == id; }), list.end());
}

}  // namespace EventDistributor

#endif  // EVENTDISTRIBUTOR_H
