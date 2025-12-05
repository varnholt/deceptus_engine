#pragma once

#include <SFML/Graphics.hpp>

#include <chrono>
#include <functional>
#include <future>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

class EventSerializer
{
public:
   EventSerializer() = default;

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

   // Static registry methods for named instances
   static std::shared_ptr<EventSerializer> getInstance(const std::string& name);
   static void registerInstance(const std::string& name, const std::shared_ptr<EventSerializer>& instance);
   static void unregisterInstance(const std::string& name);

   void add(const sf::Event& event);
   void clear();

   void serialize();
   void deserialize(const std::filesystem::path& path = "events.dat");

   void debug();
   void play();

   void update(sf::Time dt);  // New time-based update function
   void setCallback(const EventCallback& callback);

   std::optional<size_t> getMaxSize() const;
   void setMaxSize(size_t max_size);

   bool isEnabled() const;
   void setEnabled(bool enabled);

   bool isPlaying() const;    // New function to check if actively playing

   void start();       //!< convenience helper function
   void stop();        //!< convenience helper function

   static void addToAll(const sf::Event& event);
   static void updateAll(sf::Time dt);

private:

   static bool filterMovementEvents(const sf::Event& event);

   // Static registry for named instances
   static std::unordered_map<std::string, std::weak_ptr<EventSerializer>> _instance_registry;

   std::optional<size_t> _max_size;
   std::vector<ChronoEvent> _events;
   bool _playing = false;              //!< New field to track playback state
   sf::Time _elapsed_time;             //!< New field to track elapsed time during playback
   size_t _current_event_index = 0;    //!< New field to track current event during playback
   bool _enabled = false;

   EventCallback _callback;
};
