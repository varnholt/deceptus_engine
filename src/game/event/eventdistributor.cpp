#include "eventdistributor.h"
#include <algorithm>
#include <typeindex>
#include <unordered_map>

namespace
{
using event_variant = sf::Event;
std::unordered_map<std::type_index, std::vector<std::function<void(const sf::Event&)>>> callback_mapping;
}  // namespace

void EventDistributor::event(const sf::Event& event)
{
   std::visit(
      [](auto&& e)
      {
         using event_t = std::decay_t<decltype(e)>;
         auto type_id = std::type_index(typeid(event_t));

         auto it = callback_mapping.find(type_id);
         if (it != callback_mapping.end())
         {
            for (const auto& callback : it->second)
            {
               callback(e);
            }
         }
      },
      event
   );
}

template <typename EventT>
void EventDistributor::registerEvent(const EventCallback<EventT>& callback)
{
   auto type_id = std::type_index(typeid(EventT));
   callback_mapping[type_id].emplace_back(callback);
}

template <typename EventT>
void EventDistributor::unregisterEvent(const EventCallback<EventT>& callback)
{
   auto type_id = std::type_index(typeid(EventT));

   auto it = callback_mapping.find(type_id);
   if (it != callback_mapping.end())
   {
      auto& callback_list = it->second;
      callback_list.erase(
         std::remove_if(
            callback_list.begin(),
            callback_list.end(),
            [&callback](const std::function<void(const sf::Event&)>& cb)
            { return cb.target<void(const EventT&)>() == callback.target<void(const EventT&)>(); }
         ),
         callback_list.end()
      );

      if (callback_list.empty())
      {
         callback_mapping.erase(it);
      }
   }
}

template void EventDistributor::registerEvent<sf::Event::KeyPressed>(const EventCallback<sf::Event::KeyPressed>&);
template void EventDistributor::registerEvent<sf::Event::MouseButtonPressed>(const EventCallback<sf::Event::MouseButtonPressed>&);
template void EventDistributor::registerEvent<sf::Event::MouseWheelScrolled>(const EventCallback<sf::Event::MouseWheelScrolled>&);

template void EventDistributor::unregisterEvent<sf::Event::KeyPressed>(const EventCallback<sf::Event::KeyPressed>&);
template void EventDistributor::unregisterEvent<sf::Event::MouseButtonPressed>(const EventCallback<sf::Event::MouseButtonPressed>&);
template void EventDistributor::unregisterEvent<sf::Event::MouseWheelScrolled>(const EventCallback<sf::Event::MouseWheelScrolled>&);
