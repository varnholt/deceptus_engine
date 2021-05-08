#pragma once

#include <SFML/Graphics.hpp>

#include <chrono>
#include <vector>

class EventSerializer
{
   public:

      struct ChronoEvent
      {
         std::chrono::time_point<std::chrono::steady_clock> _time_point;
         sf::Event _event;
      };

      EventSerializer() = default;

      void add(const sf::Event& event);
      void clear();

      void serialize();
      void deserialize();

      void play();


   private:

      void playThread();

      std::vector<ChronoEvent> _events;
};

