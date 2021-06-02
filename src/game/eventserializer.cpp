#include "eventserializer.h"

#include <iostream>
#include <ostream>
#include <fstream>
#include <thread>


void writeInt32(std::ofstream& stream, int32_t value)
{
   stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
}


int32_t readInt32(std::ifstream& stream)
{
   int32_t value = 0;
   stream.read(reinterpret_cast<char*>(&value), sizeof(value));
   return value;
}


void writeUInt8(std::ofstream& stream, uint8_t value)
{
   stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
}


uint8_t readUint8(std::ifstream& stream)
{
   uint8_t value = 0;
   stream.read(reinterpret_cast<char*>(&value), sizeof(value));
   return value;
}


void writeTimePoint(std::ofstream& stream, const std::chrono::high_resolution_clock::time_point& time_point)
{
   using namespace std::chrono_literals;
   auto const cache_time = (time_point + 12h).time_since_epoch().count();
   stream.write(reinterpret_cast<char const*>(&cache_time), sizeof cache_time);
}


std::chrono::high_resolution_clock::time_point readTimePoint(std::ifstream& stream)
{
   std::chrono::system_clock::rep file_time_rep;
   stream.read(reinterpret_cast<char*>(&file_time_rep), sizeof(file_time_rep));
   std::chrono::high_resolution_clock::time_point time_point{std::chrono::high_resolution_clock::time_point::duration{file_time_rep}};
   return time_point;
}


void EventSerializer::add(const sf::Event& event)
{
   const auto now = std::chrono::high_resolution_clock::now();
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


void writeEvent(std::ofstream& stream, const sf::Event& event)
{
   switch (event.type)
   {
      case sf::Event::KeyPressed:
      case sf::Event::KeyReleased:
      {
         const auto key_event = event.key;

         const uint8_t flags =
              (key_event.alt     << 3)
            | (key_event.control << 2)
            | (key_event.shift   << 1)
            | (key_event.system  << 0);

         writeUInt8(stream, static_cast<uint8_t>(event.type));
         writeUInt8(stream, static_cast<uint8_t>(key_event.code));
         writeUInt8(stream, flags);
         break;
      }
      default:
      {
         break;
      }
   }
}


sf::Event readEvent(std::ifstream& stream)
{
   sf::Event event;

   event.type = static_cast<sf::Event::EventType>(readUint8(stream));

   switch (event.type)
   {
      case sf::Event::KeyPressed:
      case sf::Event::KeyReleased:
      {
         auto key_code = readUint8(stream);
         auto flags = readUint8(stream);

         event.key.code = static_cast<sf::Keyboard::Key>(key_code);
         event.key.alt     = (flags & 0x08);
         event.key.control = (flags & 0x04);
         event.key.shift   = (flags & 0x02);
         event.key.system  = (flags & 0x01);
         break;
      }

      default:
      {
         break;
      }
   }

   return event;
}


void EventSerializer::serialize()
{
   std::ofstream out("events.dat", std::ios::out | std::ios::binary);

   writeInt32(out, static_cast<int32_t>(_events.size()));

   for (auto& event : _events)
   {
      writeTimePoint(out, event._time_point);
      writeEvent(out, event._event);
   }
}


void EventSerializer::deserialize()
{
   std::ifstream in("events.dat", std::ios::in | std::ios::binary);

   int32_t size = readInt32(in);

   for (auto i = 0; i < size; i++)
   {
      const auto time_point = readTimePoint(in);
      const auto event = readEvent(in);
      _events.push_back({time_point, event});
   }
}


void EventSerializer::play()
{
   const auto start = _events.at(0)._time_point;

   for (const auto& event : _events)
   {
      const auto& time_point = event._time_point;
      const auto dt = time_point - start;

      std::cout << dt.count() << std::endl;
   }
}



void EventSerializer::playThread()
{
   using namespace std::chrono_literals;

   const auto start_timepoint_current = std::chrono::high_resolution_clock::now();
   const auto start_timepoint_recorded = _events.at(0)._time_point;

   auto done = false;

   int32_t recorded_index = 0;

   while (!done)
   {
      const auto current_time_point = std::chrono::high_resolution_clock::now();
      const auto current_duration = current_time_point - start_timepoint_current;
      const auto duration_to_next_index = _events.at(recorded_index)._time_point - start_timepoint_recorded;

      if (current_duration > duration_to_next_index)
      {
         recorded_index++;
         done = (recorded_index == static_cast<int32_t>(_events.size() - 1));
      }

      std::this_thread::sleep_for(1ms);
   }
}


