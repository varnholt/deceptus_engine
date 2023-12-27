#ifndef KEYBOARDEVENTDISTRIBUTOR_H
#define KEYBOARDEVENTDISTRIBUTOR_H

#include <functional>
#include <sfml/Graphics.hpp>

namespace EventDistributor
{
using EventCallback = std::function<void(const sf::Event&)>;
void event(const sf::Event& event);
void registerEvent(sf::Event::EventType event_type, const EventCallback& callback);
void unregisterEvent(sf::Event::EventType event_type, const EventCallback& callback);
};  // namespace EventDistributor

#endif  // KEYBOARDEVENTDISTRIBUTOR_H
