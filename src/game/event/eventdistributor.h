#ifndef EVENTDISTRIBUTOR_H
#define EVENTDISTRIBUTOR_H

#include <SFML/Window/Event.hpp>
#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <vector>

/**
 * @file eventdistributor.h
 * @brief A lightweight, thread-safe generic event distributor system.
 *
 * Allows registering callbacks for any event type and dispatching events
 * to all registered listeners. Each callback registration returns an ID
 * which can be used to unregister it later.
 */

namespace EventDistributor
{

/**
 * @brief Type alias for event callback functions.
 *
 * @tparam EventT The type of event this callback handles.
 */
template <typename EventT>
using EventCallback = std::function<void(const EventT&)>;

/**
 * @brief Generates a unique callback ID.
 *
 * Thread-safe monotonic counter to assign unique IDs to registered callbacks.
 *
 * @return A unique int32_t ID.
 */
inline int32_t getNextCallbackId()
{
   static std::atomic<int32_t> current_id{0};
   return ++current_id;
}

/**
 * @brief Internal type storing a callback ID and its corresponding function.
 *
 * @tparam EventT The event type associated with this callback.
 */
template <typename EventT>
using CallbackEntry = std::pair<int32_t, EventCallback<EventT>>;

/**
 * @brief Provides access to the internal list of callback entries for a specific event type.
 *
 * @tparam EventT The event type.
 * @return Reference to the vector of registered callback entries.
 */
template <typename EventT>
std::vector<CallbackEntry<EventT>>& getCallbackList()
{
   static std::vector<CallbackEntry<EventT>> callbacks;
   return callbacks;
}

/**
 * @brief Provides access to the mutex protecting the callback list for a specific event type.
 *
 * @tparam EventT The event type.
 * @return Reference to the mutex guarding the callback list.
 */
template <typename EventT>
std::mutex& getCallbackMutex()
{
   static std::mutex mtx;
   return mtx;
}

/**
 * @brief Dispatches an event to all registered callbacks for the given event type.
 *
 * Thread-safe. All registered callbacks for the given type are invoked
 * in the order they were registered.
 *
 * @tparam EventT The event type.
 * @param event The event instance to dispatch.
 */
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

/**
 * @brief Registers a callback for the given event type.
 *
 * Thread-safe. The callback will be called whenever an event of this type
 * is dispatched using event().
 *
 * @tparam EventT The event type.
 * @param callback The callback function to register.
 * @return A unique int32_t ID that can be used to unregister the callback.
 */
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

/**
 * @brief Unregisters a previously registered callback by its ID.
 *
 * Thread-safe. If no callback with the given ID exists, this function does nothing.
 *
 * @tparam EventT The event type.
 * @param id The ID of the callback to unregister.
 */
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
