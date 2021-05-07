#include "eventserializer.h"

#include <ostream>
#include <fstream>


void EventSerializer::add(const sf::Event& event)
{
   const auto now = std::chrono::steady_clock::now();
   _events.push_back({now, event});

   if (_events.size() == 100)
   {
      serialize();
   }
}


void EventSerializer::clear()
{
   _events.clear();
}


void EventSerializer::serialize()
{
   std::ofstream out("events.dat", std::ios::out | std::ios::binary);

   out << static_cast<int32_t>(_events.size());

   for (auto& event : _events)
   {
      out << event._time_point.time_since_epoch().count();
   }
}


void EventSerializer::deserialize()
{
   std::ifstream in("events.dat", std::ios::in | std::ios::binary);

   int32_t size = 0;

   in >> size;

   std::chrono::steady_clock::rep file_time_rep;

   for (auto i = 0; i < size; i++)
   {
      in >> file_time_rep;
      std::chrono::steady_clock::time_point time_point{std::chrono::steady_clock::time_point::duration{file_time_rep}};
      _events.push_back({time_point, sf::Event()});
   }
}

