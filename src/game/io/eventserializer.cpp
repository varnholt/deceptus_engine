#include "eventserializer.h"

#include "framework/tools/log.h"
#include "game/state/displaymode.h"
#include "game/state/gamestate.h"

#include <fstream>
#include <iostream>
#include <ostream>
#include <thread>
#include <unordered_set>

namespace
{
using HighResDuration = std::chrono::high_resolution_clock::duration;
using HighResTimePoint = std::chrono::high_resolution_clock::time_point;
using HighResClockRep = std::chrono::high_resolution_clock::rep;
using HighResClock = std::chrono::high_resolution_clock;

constexpr uint8_t EVENT_KEY_PRESSED = 1;
constexpr uint8_t EVENT_KEY_RELEASED = 2;
constexpr uint8_t EVENT_UNKNOWN = 255;

}  // namespace

template <typename R>
static bool is_ready(std::future<R> const& f)
{
   return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

void writeInt32(std::ostream& stream, int32_t value)
{
   stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

int32_t readInt32(std::istream& stream)
{
   int32_t value = 0;
   stream.read(reinterpret_cast<char*>(&value), sizeof(value));
   return value;
}

void writeUInt8(std::ostream& stream, uint8_t value)
{
   stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

uint8_t readUint8(std::istream& stream)
{
   uint8_t value = 0;
   stream.read(reinterpret_cast<char*>(&value), sizeof(value));
   return value;
}

void writeTimePoint(std::ostream& stream, const HighResTimePoint& time_point)
{
   using namespace std::chrono_literals;
   auto const time_point_raw = (time_point + 12h).time_since_epoch().count();
   stream.write(reinterpret_cast<char const*>(&time_point_raw), sizeof time_point_raw);
}

void writeDuration(std::ostream& stream, const HighResDuration& duration)
{
   auto const duration_raw = duration.count();
   stream.write(reinterpret_cast<char const*>(&duration_raw), sizeof duration_raw);
}

HighResTimePoint readTimePoint(std::istream& stream)
{
   HighResClockRep file_time_rep;
   stream.read(reinterpret_cast<char*>(&file_time_rep), sizeof(file_time_rep));
   HighResTimePoint time_point{HighResDuration{file_time_rep}};
   return time_point;
}

HighResDuration readDuration(std::istream& stream)
{
   HighResClockRep file_time_rep;
   stream.read(reinterpret_cast<char*>(&file_time_rep), sizeof(file_time_rep));
   auto duration = HighResDuration{file_time_rep};
   return duration;
}

void EventSerializer::add(const sf::Event& event)
{
   if (!isEnabled())
   {
      return;
   }

   if (_play_result.has_value() && !is_ready(_play_result.value()))
   {
      return;
   }

   if (GameState::getInstance().getMode() != ExecutionMode::Running)
   {
      return;
   }

   if (_max_size.has_value() && _events.size() >= _max_size)
   {
      return;
   }

   if (!filterMovementEvents(event))
   {
      return;
   }

   const auto now = HighResClock::now();
   _events.emplace_back(now, event);
}

void EventSerializer::clear()
{
   _events.clear();
}

void writeEvent(std::ostream& stream, const sf::Event& event)
{
   event.visit(
      [&](const auto& e)
      {
         using event_t = std::decay_t<decltype(e)>;

         constexpr uint8_t event_id = []
         {
            if constexpr (std::is_same_v<event_t, sf::Event::KeyPressed>)
            {
               return EVENT_KEY_PRESSED;
            }
            if constexpr (std::is_same_v<event_t, sf::Event::KeyReleased>)
            {
               return EVENT_KEY_RELEASED;
            }
            return EVENT_UNKNOWN;
         }();

         writeUInt8(stream, event_id);

         if constexpr (event_id == EVENT_KEY_PRESSED || event_id == EVENT_KEY_RELEASED)
         {
            writeUInt8(stream, static_cast<uint8_t>(e.code));
            uint8_t flags = (e.alt << 3) | (e.control << 2) | (e.shift << 1) | (e.system << 0);
            writeUInt8(stream, flags);
         }
         else
         {
            Log::Warning() << "writing unhandled event";
         }
      }
   );
}

sf::Event readEvent(std::istream& stream)
{
   uint8_t event_id = readUint8(stream);

   switch (event_id)
   {
      case EVENT_KEY_PRESSED:
      {
         sf::Event::KeyPressed key_event;
         key_event.code = static_cast<sf::Keyboard::Key>(readUint8(stream));
         const auto flags = readUint8(stream);
         key_event.alt = (flags & 0x08);
         key_event.control = (flags & 0x04);
         key_event.shift = (flags & 0x02);
         key_event.system = (flags & 0x01);
         return key_event;
      }
      case EVENT_KEY_RELEASED:
      {
         sf::Event::KeyReleased key_event;
         key_event.code = static_cast<sf::Keyboard::Key>(readUint8(stream));
         const auto flags = readUint8(stream);
         key_event.alt = (flags & 0x08);
         key_event.control = (flags & 0x04);
         key_event.shift = (flags & 0x02);
         key_event.system = (flags & 0x01);
         return key_event;
      }
      default:
      {
         Log::Warning() << "reading unhandled event";
      }
   }
}

void EventSerializer::serialize()
{
   if (_events.empty())
   {
      return;
   }

   Log::Info() << "serializing " << _events.size() << " events";
   std::ofstream output_stream("events.dat", std::ios::out | std::ios::binary);

   writeInt32(output_stream, static_cast<int32_t>(_events.size()));

   auto start_time = _events.front()._time_point;

   for (const auto& event : _events)
   {
      writeDuration(output_stream, event._time_point - start_time);
      writeEvent(output_stream, event._event);
   }
}

void EventSerializer::deserialize(const std::filesystem::path& path)
{
   _events.clear();

   std::ifstream input_stream(path, std::ios::in | std::ios::binary);

   const auto size = readInt32(input_stream);

   for (auto i = 0; i < size; i++)
   {
      const auto duration = readDuration(input_stream);
      const auto event = readEvent(input_stream);

      _events.emplace_back(duration, event);
   }
}

void EventSerializer::debug()
{
   const auto start = _events.at(0)._time_point;

   for (const auto& event : _events)
   {
      const auto& time_point = event._time_point;
      const auto delta_time = time_point - start;

      Log::Info() << delta_time.count();
   }
}

void EventSerializer::play()
{
   // if still busy playing, don't allow calling another time
   if (_play_result.has_value() && !is_ready(_play_result.value()))
   {
      return;
   }

   if (_events.empty())
   {
      return;
   }

   _play_start_time = HighResClock::now();
   _play_result = std::async(std::launch::async, [this] { playThread(); });

   DisplayMode::getInstance().enqueueSet(Display::ReplayPlaying);
}

void EventSerializer::playThread()
{
   using namespace std::chrono_literals;

   auto done = false;

   int32_t recorded_index = 0;

   while (!done)
   {
      const auto now = HighResClock::now();
      const auto elapsed = now - _play_start_time;

      const auto& event = _events[recorded_index];
      if (elapsed > event._duration)
      {
         // pass event to given event loop
         _callback(event._event);

         recorded_index++;
         done = (recorded_index == static_cast<int32_t>(_events.size() - 1));
      }

      std::this_thread::sleep_for(1ms);
   }

   // not thread safe but also only for dev purposes
   DisplayMode::getInstance().enqueueUnset(Display::ReplayPlaying);
}

bool EventSerializer::filterMovementEvents(const sf::Event& event)
{
   static const std::unordered_set<sf::Keyboard::Key> movement_keys = {
      sf::Keyboard::Key::LShift,
      sf::Keyboard::Key::Left,
      sf::Keyboard::Key::Right,
      sf::Keyboard::Key::Up,
      sf::Keyboard::Key::Down,
      sf::Keyboard::Key::Enter,
      sf::Keyboard::Key::Space
   };

   if (const auto* key_event = event.getIf<sf::Event::KeyPressed>())
   {
      return movement_keys.contains(key_event->code);
   }

   if (const auto* key_event = event.getIf<sf::Event::KeyReleased>())
   {
      return movement_keys.contains(key_event->code);
   }

   return false;
}

void EventSerializer::setEnabled(bool enabled)
{
   _enabled = enabled;
}

EventSerializer& EventSerializer::getInstance()
{
   static EventSerializer _instance;
   return _instance;
}

void EventSerializer::start()
{
   DisplayMode::getInstance().enqueueSet(Display::ReplayRecording);

   clear();
   setEnabled(true);
}

void EventSerializer::stop()
{
   DisplayMode::getInstance().enqueueUnset(Display::ReplayRecording);

   setEnabled(false);
   serialize();
}

bool EventSerializer::isEnabled() const
{
   return _enabled;
}

std::optional<size_t> EventSerializer::getMaxSize() const
{
   return _max_size;
}

void EventSerializer::setMaxSize(size_t max_size)
{
   _max_size = max_size;
}

void EventSerializer::setCallback(const EventCallback& callback)
{
   _callback = callback;
}
