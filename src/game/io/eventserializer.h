#pragma once

#include <SFML/Graphics.hpp>

#include <chrono>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

/// \brief records selected sf::Event input, serializes it, and replays it with original timing.
class EventSerializer
{
public:
   EventSerializer() = default;

   using HighResDuration = std::chrono::high_resolution_clock::duration;
   using HighResTimePoint = std::chrono::high_resolution_clock::time_point;

   /// \brief stores one input event together with absolute or relative playback timing.
   struct ChronoEvent
   {
      /// \brief creates an event entry with a precomputed playback delay.
      /// \param duration elapsed time from playback start when the event should fire.
      /// \param event sfml event payload to replay.
      ChronoEvent(const HighResDuration& duration, const sf::Event& event);

      /// \brief creates an event entry stamped with recording wall-clock time.
      /// \param time_point capture timestamp at record time.
      /// \param event sfml event payload that was captured.
      ChronoEvent(const HighResTimePoint& time_point, const sf::Event& event);

      HighResTimePoint _time_point;
      HighResDuration _duration;
      sf::Event _event;
   };

   using EventCallback = std::function<void(const sf::Event& event)>;

   /// \brief returns a named serializer instance from the global registry.
   /// \param name registry key used when the instance was registered.
   /// \return shared serializer instance, or nullptr when no live instance exists.
   static std::shared_ptr<EventSerializer> getInstance(const std::string& name);

   /// \brief registers a serializer instance under a name for global fan-out operations.
   /// \param name registry key for later lookup.
   /// \param instance serializer instance to store.
   static void registerInstance(const std::string& name, const std::shared_ptr<EventSerializer>& instance);

   /// \brief removes a serializer instance from the global registry.
   /// \param name registry key to erase.
   static void unregisterInstance(const std::string& name);

   /// \brief forwards an input event to every registered serializer.
   /// \param event event to potentially record in each serializer.
   static void addToAll(const sf::Event& event);

   /// \brief advances playback state for every registered serializer.
   /// \param delta_time frame time passed to each serializer update call.
   static void updateAll(sf::Time delta_time);

   /// \brief appends a filtered input event to the recording buffer when recording is enabled.
   /// \param event event to capture.
   void add(const sf::Event& event);

   /// \brief clears all recorded or loaded events from this serializer.
   void clear();

   /// \brief writes the current event sequence to a timestamped file in the recordings directory.
   void serialize();

   /// \brief loads previously serialized event timing and payload data from a binary file.
   /// \param path path to the event recording file.
   void deserialize(const std::filesystem::path& path = "events.dat");

   /// \brief logs event timing deltas for debugging recorded input streams.
   void debug();

   /// \brief starts replay mode and schedules loaded events from time zero.
   void play();

   /// \brief dispatches due replay events through the configured callback.
   /// \param dt elapsed frame time, currently unused because timing is wall-clock based.
   void update(sf::Time dt);

   /// \brief sets the callback invoked for each replayed event.
   /// \param callback consumer that handles replayed sf::Event values.
   void setCallback(const EventCallback& callback);

   /// \brief returns the optional recording size limit.
   /// \return maximum number of events to store, or std::nullopt for no limit.
   std::optional<size_t> getMaxSize() const;

   /// \brief sets the maximum number of events recorded before add starts ignoring input.
   /// \param max_size event count cap.
   void setMaxSize(size_t max_size);

   /// \brief indicates whether recording is currently enabled.
   /// \return true when add is allowed to capture events.
   bool isEnabled() const;

   /// \brief enables or disables event recording for this serializer.
   /// \param enabled true to accept new recorded events.
   void setEnabled(bool enabled);

   /// \brief indicates whether replay playback is currently active.
   /// \return true while replay is running and events may still be dispatched.
   bool isPlaying() const;

   /// \brief enters replay-recording display mode, clears previous data, and enables capture.
   void start();

   /// \brief exits replay-recording mode, disables capture, and serializes recorded events.
   void stop();

private:
   /// \brief filters input to supported movement-related key press and key release events.
   /// \param event incoming event to test.
   /// \return true when the event should be recorded.
   static bool filterMovementEvents(const sf::Event& event);

   static std::unordered_map<std::string, std::weak_ptr<EventSerializer>> _instance_registry;

   std::optional<size_t> _max_size;
   std::vector<ChronoEvent> _events;

   /// \brief indicates whether replay playback is active.
   bool _playing = false;

   /// \brief stores elapsed replay time accumulated since play started.
   sf::Time _elapsed_time;

   /// \brief stores index of the next event that has not been replayed yet.
   size_t _current_event_index = 0;
   bool _enabled = false;

   /// \brief stores the high-resolution timestamp when replay playback began.
   HighResTimePoint _playback_start_time;

   EventCallback _callback;
};
