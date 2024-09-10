#ifndef GAMEMECHANISMOBSERVER_H
#define GAMEMECHANISMOBSERVER_H

#include <functional>
#include <memory>

namespace GameMechanismObserver
{

using EnabledCallback = std::function<void(bool)>;
static std::vector<EnabledCallback> _enabled_listeners;

template <typename Callback>
struct Reference
{
   explicit Reference(Callback callback) : _callback(std::move(callback))
   {
   }
   const Callback& _callback;
};

void onEnabled(bool enabled);

template <typename Callback>
std::unique_ptr<GameMechanismObserver::Reference<Callback>, std::function<void(GameMechanismObserver::Reference<Callback>*)>> addListener(
   const Callback& callback_to_add
)
{
   auto custom_deleter = [](GameMechanismObserver::Reference<Callback>* reference)
   {
      const auto& callback_to_remove = reference->_callback;

      if constexpr (std::is_same_v<Callback, EnabledCallback>)
      {
         std::erase_if(_enabled_listeners, [&callback_to_remove](const auto& callback) { return &callback == &callback_to_remove; });
      }
      delete reference;
   };

   if constexpr (std::is_same_v<Callback, EnabledCallback>)
   {
      _enabled_listeners.emplace_back(callback_to_add);
   }

   return std::unique_ptr<GameMechanismObserver::Reference<Callback>, decltype(custom_deleter)>(
      new GameMechanismObserver::Reference(callback_to_add), custom_deleter
   );
}
};

#endif  // GAMEMECHANISMOBSERVER_H
