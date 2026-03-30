#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

class JoystickHandler;
class GameController;
class GameControllerDetection;

/// \brief manages SDL controller devices and publishes add/remove notifications.
class GameControllerIntegration
{
public:
   GameControllerIntegration() = default;

   /// \brief releases device detection resources and shuts down SDL joystick subsystems.
   virtual ~GameControllerIntegration();

   /// \brief initializes SDL controller support, mapping database, and device hotplug detection.
   void initialize();

   /// \brief processes queued device-change actions on the main thread.
   void update();

   /// \brief returns the global controller integration manager.
   /// \return singleton integration instance.
   static GameControllerIntegration& getInstance();

   /// \brief returns how many controllers are currently active.
   /// \return number of entries in the managed controller map.
   size_t getCount() const;

   /// \brief checks whether at least one controller is connected.
   /// \return true when the controller map is not empty.
   bool isControllerConnected() const;

   /// \brief returns a managed controller object by id.
   /// \param controller_id joystick id key used in the internal controller map.
   /// \return shared pointer reference for the requested controller.
   const std::shared_ptr<GameController>& getController(int32_t controller_id = _selected_controller_id) const;

   using DeviceAddedCallback = std::function<void(int32_t)>;
   using DeviceRemovedCallback = std::function<void(int32_t)>;
   using DeviceChangedCallback = std::function<void(void)>;

   /// \brief registers a callback for controller-added events and replays it for existing devices.
   /// \param callback handler called with each connected controller id.
   void addDeviceAddedCallback(const DeviceAddedCallback& callback);

   /// \brief registers a callback for controller-removed events.
   /// \param callback handler called with each removed controller id.
   void addDeviceRemovedCallback(const DeviceAddedCallback& callback);

private:
   /// \brief queues controller activation and add-callback notification for a joystick id.
   /// \param id joystick id reported by SDL hotplug events.
   void add(int32_t id);

   /// \brief queues controller removal and remove-callback notification for a joystick id.
   /// \param id joystick id reported by SDL hotplug events.
   void remove(int32_t id);

   std::unique_ptr<GameControllerDetection> _device_detection;
   std::map<int32_t, std::shared_ptr<GameController>> _game_controllers;

   std::vector<DeviceAddedCallback> _device_added_callbacks;
   std::vector<DeviceRemovedCallback> _device_removed_callbacks;

   std::mutex _device_changed_mutex;
   std::vector<DeviceChangedCallback> _device_changed_callbacks;

   /// this could be altered via controller menu if we want to support multiple controllers
   static int32_t _selected_controller_id;
};
