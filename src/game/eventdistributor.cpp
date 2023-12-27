#include "eventdistributor.h"

#include <algorithm>

namespace
{
std::map<sf::Event::EventType, std::vector<EventDistributor::EventCallback>> _callback_mapping;
}

void EventDistributor::event(const sf::Event& event)
{
   const auto& callbacks = _callback_mapping.find(event.type);

   if (callbacks != _callback_mapping.end())
   {
      std::for_each(callbacks->second.begin(), callbacks->second.end(), [event](const auto& callback) { callback(event); });
   }
}

void EventDistributor::registerEvent(sf::Event::EventType event_type, const EventCallback& callback)
{
   _callback_mapping[event_type].push_back(callback);
}

void EventDistributor::unregisterEvent(sf::Event::EventType event_type, const EventCallback& callback)
{
   auto& callbacks = _callback_mapping[event_type];
   callbacks.erase(
      std::remove_if(
         callbacks.begin(),
         callbacks.end(),
         [callback](const auto& cb) { return cb.target<void(const sf::Event&)>() == callback.target<void(const sf::Event&)>(); }
      ),
      callbacks.end()
   );
}
