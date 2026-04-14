#ifndef GAMEMECHANISMOBSERVER_H
#define GAMEMECHANISMOBSERVER_H

#include <functional>
#include <memory>
#include <string>
#include <variant>

#include "framework/tools/log.h"

namespace GameMechanismObserver
{

using EnabledCallback = std::function<void(const std::string&, const std::string&, bool)>;
using LuaVariant = std::variant<std::string, int64_t, double, bool>;
using EventCallback = std::function<void(const std::string&, const std::string&, const std::string&, const LuaVariant&)>;

extern std::vector<EnabledCallback> _enabled_listeners;
extern std::vector<EventCallback> _event_listeners;

/// \brief notifies listeners that a mechanism changed its enabled state.
/// \param object_id id of the mechanism that changed.
/// \param group_id mechanism group id associated with the event.
/// \param enabled true when the mechanism was enabled, false when disabled.
void onEnabled(const std::string& object_id, const std::string& group_id, bool enabled);

/// \brief broadcasts a named mechanism event with a lua-compatible payload.
/// \param object_id id of the mechanism that emitted the event.
/// \param object_group mechanism group that emitted the event.
/// \param event_name event identifier, for example "pressed".
/// \param value event payload passed to listeners.
void onEvent(const std::string& object_id, const std::string& object_group, const std::string& event_name, const LuaVariant& value);

/// \brief removes all registered enabled and event listeners.
void clear();

template <typename Callback>
/// \brief owns a callback reference used for scoped listener lifetime.
/// \tparam Callback callback type stored in the listener vector.
struct Reference
{
   /// \brief creates a reference wrapper for one callback instance.
   /// \param callback callback object to reference.
   explicit Reference(Callback callback) : _callback(std::move(callback))
   {
   }
   const Callback& _callback;
};

template <typename Callback>
/// \brief registers a listener and returns a scoped handle that unregisters it on destruction.
/// \tparam Callback callback type, either EnabledCallback or EventCallback.
/// \param callback_to_add callback to register.
/// \return owning handle that removes the callback from the observer when released.
std::unique_ptr<Reference<Callback>, std::function<void(Reference<Callback>*)>> addListener(const Callback& callback_to_add)
{
   auto custom_deleter = [](Reference<Callback>* reference)
   {
      const auto& callback_to_remove = reference->_callback;

      if constexpr (std::is_same_v<Callback, EnabledCallback>)
      {
         // Log::Info() << "calling custom deleter for enabled callback";
         std::erase_if(_enabled_listeners, [&callback_to_remove](const auto& callback) { return &callback == &callback_to_remove; });
      }
      else if constexpr (std::is_same_v<Callback, EventCallback>)
      {
         // Log::Info() << "calling custom deleter for event callback";
         std::erase_if(_event_listeners, [&callback_to_remove](const auto& callback) { return &callback == &callback_to_remove; });
      }

      delete reference;
   };

   if constexpr (std::is_same_v<Callback, EnabledCallback>)
   {
      // Log::Info() << "register enabled callback";
      _enabled_listeners.push_back(callback_to_add);
   }
   else if constexpr (std::is_same_v<Callback, EventCallback>)
   {
      // Log::Info() << "register event callback";
      _event_listeners.push_back(callback_to_add);
   }

   return std::unique_ptr<Reference<Callback>, decltype(custom_deleter)>(new Reference(callback_to_add), custom_deleter);
}
};

#endif  // GAMEMECHANISMOBSERVER_H
