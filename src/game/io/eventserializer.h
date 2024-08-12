#pragma once

#include <SFML/Graphics.hpp>

#include <chrono>
#include <functional>
#include <future>
#include <optional>
#include <thread>
#include <vector>

class EventSerializer
{
public:
   using HighResDuration = std::chrono::high_resolution_clock::duration;
   using HighResTimePoint = std::chrono::high_resolution_clock::time_point;

   struct ChronoEvent
   {
      // for playback
      ChronoEvent(const HighResDuration& duration, const sf::Event& event) : _duration(duration), _event(event)
      {
      }

      // for recording
      ChronoEvent(const HighResTimePoint& time_point, const sf::Event& event) : _time_point(time_point), _event(event)
      {
      }

      HighResTimePoint _time_point;
      HighResDuration _duration;
      sf::Event _event;
   };

   using EventCallback = std::function<void(const sf::Event& event)>;

   void add(const sf::Event& event);
   void clear();

   void serialize();
   void deserialize();

   void debug();
   void play();

   void setCallback(const EventCallback& callback);

   std::optional<size_t> getMaxSize() const;
   void setMaxSize(size_t max_size);

   bool isEnabled() const;
   void setEnabled(bool enabled);

   static EventSerializer& getInstance();

private:
   EventSerializer() = default;

   void playThread();
   bool filterMovementEvents(const sf::Event& event);

   std::optional<size_t> _max_size;
   std::vector<ChronoEvent> _events;
   std::optional<std::future<void>> _play_result;
   HighResTimePoint _play_start_time;
   bool _enabled = false;

   EventCallback _callback;
};
