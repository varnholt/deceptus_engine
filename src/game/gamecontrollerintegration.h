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

class GameControllerIntegration
{
   public:

      GameControllerIntegration() = default;
      virtual ~GameControllerIntegration();

      void initialize();
      void update();

      static GameControllerIntegration& getInstance();

      size_t getCount() const;
      bool isControllerConnected() const;

      std::shared_ptr<GameController>& getController(int32_t controller_id = _selected_controller_id);

      using DeviceAddedCallback = std::function<void(int32_t)>;
      using DeviceRemovedCallback = std::function<void(int32_t)>;
      using DeviceChangedCallback = std::function<void(void)>;

      void addDeviceAddedCallback(const DeviceAddedCallback& callback);
      void addDeviceRemovedCallback(const DeviceAddedCallback& callback);


private:

      void add(int32_t id);
      void remove(int32_t id);

      std::unique_ptr<GameControllerDetection> _device_detection;
      std::map<int32_t, std::shared_ptr<GameController>> _controllers;

      std::vector<DeviceAddedCallback> _device_added_callbacks;
      std::vector<DeviceRemovedCallback> _device_removed_callbacks;

      std::mutex _device_changed_mutex;
      std::vector<DeviceChangedCallback> _device_changed_callbacks;

      //! this could be altered via controller menu if we want to support multiple controllers
      static int32_t _selected_controller_id;
};

