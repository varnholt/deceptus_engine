#ifndef KEYBOARDEVENTDISTRIBUTOR_H
#define KEYBOARDEVENTDISTRIBUTOR_H

#include <functional>
#include <SFML/Graphics.hpp>

namespace EventDistributor
{
using EventCallbackType = void(const sf::Event&);
using EventCallback = std::function<EventCallbackType>;
using CallbackWrapper = std::reference_wrapper<const EventCallback>;

void event(const sf::Event& event);
void registerEvent(sf::Event::EventType event_type, const EventCallback& callback);
void unregisterEvent(sf::Event::EventType event_type, const EventCallback& callback);
};  // namespace EventDistributor

#endif  // KEYBOARDEVENTDISTRIBUTOR_H
