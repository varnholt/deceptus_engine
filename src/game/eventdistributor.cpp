#include "eventdistributor.h"

#include <algorithm>
#include <functional>
#include <map>

namespace
{
std::map<sf::Event::EventType, std::vector<EventDistributor::CallbackWrapper>> _callback_mapping;
}

void EventDistributor::event(const sf::Event& event)
{
   const auto& callbacks = _callback_mapping.find(event.type);

   if (callbacks != _callback_mapping.end())
   {
      std::for_each(callbacks->second.begin(), callbacks->second.end(), [event](const auto& callback) { callback.get()(event); });
   }
}

void EventDistributor::registerEvent(sf::Event::EventType event_type, const EventCallback& callback)
{
   _callback_mapping[event_type].emplace_back(std::cref(callback));
}

void EventDistributor::unregisterEvent(sf::Event::EventType event_type, const EventCallback& callback)
{
   auto& callbacks = _callback_mapping[event_type];
   callbacks.erase(
      std::remove_if(
         callbacks.begin(),
         callbacks.end(),
         [&callback](const CallbackWrapper& cb)
         {
            // compare reference pointers
            return &cb.get() == &callback;
         }
      ),
      callbacks.end()
   );
}
