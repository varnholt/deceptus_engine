#include "eventserializer.h"

#include "framework/tools/gamepaths.h"
#include "framework/tools/log.h"
#include "framework/tools/sfmlcompat.h"
#include "game/config/inputconfiguration.h"
#include "game/player/playerregistry.h"
#include "game/state/displaymode.h"
#include "game/state/gamestate.h"

#include <fstream>
#include <iostream>
#include <memory>
#include <ostream>
#include <sstream>
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

std::unordered_map<std::string, std::weak_ptr<EventSerializer>> EventSerializer::_instance_registry;

HighResTimePoint EventSerializer::elapsedTimePoint() const
{
   return HighResTimePoint{std::chrono::duration_cast<HighResDuration>(std::chrono::microseconds(_elapsed_time.asMicroseconds()))};
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

void writeFloat(std::ostream& stream, float value)
{
   stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

float readFloat(std::istream& stream)
{
   float value = 0.0f;
   stream.read(reinterpret_cast<char*>(&value), sizeof(value));
   return value;
}

void writeUInt16(std::ostream& stream, uint16_t value)
{
   stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

uint16_t readUint16(std::istream& stream)
{
   uint16_t value = 0;
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

   if (_playing)
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

   if (!filterPlayerInputEvents(event))
   {
      return;
   }

   _events.emplace_back(elapsedTimePoint(), event);
}

void EventSerializer::clear()
{
   _events.clear();
}

void writeEvent(std::ostream& stream, const sf::Event& event)
{
   event.visit(
      [&](const auto& visited_event)
      {
         using event_t = std::decay_t<decltype(visited_event)>;

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
            // what is written is the action the key stood for, not the key. a recording outlives the
            // bindings it was made under, and a player who moves jump off space would otherwise get
            // a demo that never jumps - or one that opens the inventory instead
            const auto& key_to_action = InputConfiguration::getInstance()._key_to_action;
            const auto action_it = key_to_action.find(visited_event.code);
            writeUInt16(stream, action_it != key_to_action.end() ? static_cast<uint16_t>(action_it->second) : 0);
         }
         else
         {
            Log::Warning() << "writing unhandled event";
         }
      }
   );
}

/// \brief reads one recorded action and resolves it to the key it is bound to right now.
/// \param stream input stream positioned at the action field.
/// \return key the action is currently bound to, or Unknown when it is not bound at all.
sf::Keyboard::Key readActionKey(std::istream& stream)
{
   const auto action = static_cast<KeyPressed>(readUint16(stream));
   const auto& action_to_key = InputConfiguration::getInstance()._action_to_key;
   const auto key_it = action_to_key.find(action);
   return key_it != action_to_key.end() ? key_it->second : sf::Keyboard::Key::Unknown;
}

sf::Event readEvent(std::istream& stream)
{
   uint8_t event_id = readUint8(stream);

   switch (event_id)
   {
      case EVENT_KEY_PRESSED:
      {
         sf::Event::KeyPressed key_event;
         key_event.code = readActionKey(stream);
         return key_event;
      }
      case EVENT_KEY_RELEASED:
      {
         sf::Event::KeyReleased key_event;
         key_event.code = readActionKey(stream);
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

   // generate filename with current date and time
   const auto now = std::chrono::system_clock::now();
   const auto now_time = std::chrono::system_clock::to_time_t(now);
   std::stringstream filename_stream;
   filename_stream << std::put_time(std::localtime(&now_time), "recording-%Y-%m-%d__%H-%M-%S.dat");
   const auto filename = filename_stream.str();
   const auto recording_path = GamePaths::getRecordingDir() / filename;

   Log::Info() << "serializing " << _events.size() << " events to " << recording_path;
   std::ofstream output_stream(recording_path, std::ios::out | std::ios::binary);

   const auto start_position_px = _start_position_px.value_or(sf::Vector2f{});
   writeFloat(output_stream, start_position_px.x);
   writeFloat(output_stream, start_position_px.y);
   writeInt32(output_stream, static_cast<int32_t>(_events.size()));

   auto start_time = _events.front()._time_point;

   for (auto& event : _events)
   {
      event._duration = event._time_point - start_time;
      writeDuration(output_stream, event._duration);
      writeEvent(output_stream, event._event);
   }
}

void EventSerializer::deserialize(const std::filesystem::path& path)
{
   _events.clear();

   std::ifstream input_stream(path, std::ios::in | std::ios::binary);

   const auto start_position_x_px = readFloat(input_stream);
   const auto start_position_y_px = readFloat(input_stream);
   _start_position_px = sf::Vector2f{start_position_x_px, start_position_y_px};

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

void EventSerializer::play(StartPosition start_position)
{
   // if still busy playing, don't allow calling another time
   if (_playing)
   {
      return;
   }

   if (_events.empty())
   {
      return;
   }

   if (start_position == StartPosition::Apply && _start_position_px.has_value())
   {
      const auto& player = PlayerRegistry::getFirst();
      if (player)
      {
         player->setBodyViaPixelPosition(_start_position_px->x, _start_position_px->y);
      }
   }

   Log::Info() << "re-playing " << _events.size() << " events";

   _playing = true;
   _elapsed_time = sfcompat::timeZero();
   _current_event_index = 0;

   DisplayMode::getInstance().enqueueSet(Display::ReplayPlaying);
}

void EventSerializer::update(sf::Time delta_time)
{
   // update is called once per simulation step, so this is the clock both ends of a recording run
   // on: add stamps events with it and the loop below replays them against it. it is advanced here
   // for recording serializers as well, which is why it sits ahead of the playback check
   _elapsed_time += delta_time;

   if (!_playing || _events.empty() || !_callback)
   {
      return;
   }

   const auto elapsed_duration = elapsedTimePoint().time_since_epoch();

   while (_current_event_index < _events.size())
   {
      const auto& event = _events[_current_event_index];

      // strictly greater, not equal: an event is stamped with the time that had elapsed when it
      // arrived, and the step running at that moment had already been computed without it - the key
      // first took effect on the step after. firing on equality replays every input one step early,
      // which is a constant 3.4px lead at running speed and a missed ledge at the wrong moment
      if (elapsed_duration > event._duration)
      {
         Log::Info() << "play event " << _current_event_index << " duration: " << event._duration.count();
         _callback(event._event);
         _current_event_index++;
      }
      else
      {
         break;
      }
   }

   if (_current_event_index >= _events.size())
   {
      _playing = false;
      DisplayMode::getInstance().enqueueUnset(Display::ReplayPlaying);
   }
}

bool EventSerializer::isPlaying() const
{
   return _playing;
}

void EventSerializer::stopPlayback()
{
   if (!_playing)
   {
      return;
   }

   _playing = false;

   DisplayMode::getInstance().enqueueUnset(Display::ReplayPlaying);
}

bool EventSerializer::filterPlayerInputEvents(const sf::Event& event)
{
   // whatever the player is bound to is what a replay has to reproduce, so the set comes from the
   // bindings rather than from a list of its own. a hardcoded one left out the two item slots and
   // the inventory, and a recording then played back without a single attack in it
   const auto& key_to_action = InputConfiguration::getInstance()._key_to_action;

   if (const auto* key_event = event.getIf<sf::Event::KeyPressed>())
   {
      return key_to_action.find(key_event->code) != key_to_action.end();
   }

   if (const auto* key_event = event.getIf<sf::Event::KeyReleased>())
   {
      return key_to_action.find(key_event->code) != key_to_action.end();
   }

   return false;
}

void EventSerializer::setEnabled(bool enabled)
{
   // a recording holds key events and nothing else, so replaying it only lands where it was meant to
   // when it starts from the spot it was captured at. this is where every recording path comes
   // through - the f8 hotkey as well as the console's playback commands - so the position is taken here
   if (enabled && !_enabled)
   {
      const auto& player = PlayerRegistry::getFirst();
      if (player)
      {
         _start_position_px = player->getPixelPositionFloat();
      }
   }

   _enabled = enabled;
}

void EventSerializer::start()
{
   DisplayMode::getInstance().enqueueSet(Display::ReplayRecording);

   clear();
   _elapsed_time = sfcompat::timeZero();
   setEnabled(true);
}

const std::optional<sf::Vector2f>& EventSerializer::getStartPosition() const
{
   return _start_position_px;
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

std::shared_ptr<EventSerializer> EventSerializer::getInstance(const std::string& name)
{
   auto it = _instance_registry.find(name);
   if (it != _instance_registry.end())
   {
      return it->second.lock();
   }
   return nullptr;
}

void EventSerializer::registerInstance(const std::string& name, const std::shared_ptr<EventSerializer>& instance)
{
   _instance_registry[name] = instance;
}

void EventSerializer::unregisterInstance(const std::string& name)
{
   _instance_registry.erase(name);
}

void EventSerializer::setCallback(const EventCallback& callback)
{
   _callback = callback;
}

void EventSerializer::addToAll(const sf::Event& event)
{
   for (auto& pair : _instance_registry)
   {
      if (auto instance = pair.second.lock())
      {
         instance->add(event);
      }
   }
}

void EventSerializer::updateAll(sf::Time delta_time)
{
   for (auto& pair : _instance_registry)
   {
      if (auto instance = pair.second.lock())
      {
         instance->update(delta_time);
      }
   }
}

EventSerializer::ChronoEvent::ChronoEvent(const HighResDuration& duration, const sf::Event& event) : _duration(duration), _event(event)
{
}

EventSerializer::ChronoEvent::ChronoEvent(const HighResTimePoint& time_point, const sf::Event& event)
    : _time_point(time_point), _event(event)
{
}
