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
