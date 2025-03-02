#include "eventdistributor.h"
#include <algorithm>
#include <typeindex>
#include <unordered_map>

namespace
{
// Store callbacks indexed by event type
std::unordered_map<std::type_index, std::vector<std::function<void(const sf::Event&)>>> _callback_mapping;
}  // namespace

template <typename EventT>
void EventDistributor::event(const EventT& event)
{
   auto typeId = std::type_index(typeid(EventT));

   auto it = _callback_mapping.find(typeId);
   if (it != _callback_mapping.end())
   {
      for (const auto& callback : it->second)
      {
         callback(event);
      }
   }
}

template <typename EventT>
void EventDistributor::registerEvent(EventCallback<EventT> callback)
{
   auto typeId = std::type_index(typeid(EventT));
   _callback_mapping[typeId].push_back(
      [callback](const sf::Event& event)
      {
         if (const auto* specific_event = event.getIf<EventT>())
         {
            callback(*specific_event);
         }
      }
   );
}

template <typename EventT>
void EventDistributor::unregisterEvent(EventCallback<EventT> callback)
{
   auto typeId = std::type_index(typeid(EventT));

   auto it = _callback_mapping.find(typeId);
   if (it != _callback_mapping.end())
   {
      auto& callback_list = it->second;
      callback_list.erase(
         std::remove_if(
            callback_list.begin(),
            callback_list.end(),
            [&callback](const std::function<void(const sf::Event&)>& stored_callback)
            { return stored_callback.target<void(const EventT&)>() == callback.target<void(const EventT&)>(); }
         ),
         callback_list.end()
      );

      if (callback_list.empty())
      {
         _callback_mapping.erase(it);
      }
   }
}

// Explicit template instantiations for common events
template void EventDistributor::registerEvent<sf::Event::KeyPressed>(EventCallback<sf::Event::KeyPressed>);
template void EventDistributor::registerEvent<sf::Event::KeyReleased>(EventCallback<sf::Event::KeyReleased>);
template void EventDistributor::registerEvent<sf::Event::MouseButtonPressed>(EventCallback<sf::Event::MouseButtonPressed>);
template void EventDistributor::registerEvent<sf::Event::MouseWheelScrolled>(EventCallback<sf::Event::MouseWheelScrolled>);

template void EventDistributor::unregisterEvent<sf::Event::KeyPressed>(EventCallback<sf::Event::KeyPressed>);
template void EventDistributor::unregisterEvent<sf::Event::KeyReleased>(EventCallback<sf::Event::KeyReleased>);
template void EventDistributor::unregisterEvent<sf::Event::MouseButtonPressed>(EventCallback<sf::Event::MouseButtonPressed>);
template void EventDistributor::unregisterEvent<sf::Event::MouseWheelScrolled>(EventCallback<sf::Event::MouseWheelScrolled>);
