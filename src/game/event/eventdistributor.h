#ifndef EVENTDISTRIBUTOR_H
#define EVENTDISTRIBUTOR_H

#include <SFML/Graphics.hpp>
#include <functional>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace EventDistributor
{
// Global callback mapping
static std::unordered_map<std::type_index, std::vector<std::function<void(const sf::Event&)>>> _callback_mapping;

template <typename EventT>
using EventCallback = std::function<void(const EventT&)>;

// Event function definitions

template <typename EventT>
void event(const EventT& event)
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
void registerEvent(EventCallback<EventT> callback)
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
void unregisterEvent(EventCallback<EventT> callback)
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
}  // namespace EventDistributor

#endif  // EVENTDISTRIBUTOR_H
