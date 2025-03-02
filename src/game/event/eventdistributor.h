#ifndef EVENTDISTRIBUTOR_H
#define EVENTDISTRIBUTOR_H

#include <SFML/Graphics.hpp>
#include <functional>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace EventDistributor
{
template <typename EventT>
using EventCallback = std::function<void(const EventT&)>;

template <typename EventT>
void event(const EventT& event);

template <typename EventT>
void registerEvent(EventCallback<EventT> callback);

template <typename EventT>
void unregisterEvent(EventCallback<EventT> callback);
}  // namespace EventDistributor

#endif  // EVENTDISTRIBUTOR_H
