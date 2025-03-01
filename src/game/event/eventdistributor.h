#ifndef KEYBOARD_EVENT_DISTRIBUTOR_H
#define KEYBOARD_EVENT_DISTRIBUTOR_H

#include <SFML/Graphics.hpp>
#include <functional>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace EventDistributor
{
template <typename EventT>
using EventCallback = std::function<void(const EventT&)>;

void event(const sf::Event& event);

template <typename EventT>
void registerEvent(const EventCallback<EventT>& callback);

template <typename EventT>
void unregisterEvent(const EventCallback<EventT>& callback);
};  // namespace event_distributor

#endif  // KEYBOARD_EVENT_DISTRIBUTOR_H
