#pragma once

#include <cstdint>
#include <map>
#include <memory>

class JoystickHandler;
class GameController;
class GameControllerDetection;

class GameControllerIntegration
{
   public:

      GameControllerIntegration() = default;
      void initialize();

      static GameControllerIntegration& getInstance();

      int32_t getCount() const;
      bool isControllerConnected() const;

      void initializeController(int32_t id = 0);
      std::shared_ptr<GameController>& getController(int32_t controller_id = 0);
      void rumble(float intensity, int32_t ms, int32_t controller_id = 0);


private:

      void add(int32_t id);
      void remove(int32_t id);

      std::unique_ptr<GameControllerDetection> _device_detection;
      std::map<int32_t, std::shared_ptr<GameController>> _controllers;
};

