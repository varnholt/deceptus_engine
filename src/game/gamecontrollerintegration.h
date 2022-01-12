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

      GameControllerIntegration();
      virtual ~GameControllerIntegration() = default;

      static int32_t initializeAll();

      static GameControllerIntegration* getInstance(int32_t id);

      static int32_t getCount();
      static bool isControllerConnected();

      void initialize(int32_t id = 0);
      GameController* getController();
      void rumble(float intensity, int32_t ms);


private:

      GameController* _controller = nullptr;
      static std::unique_ptr<GameControllerDetection> _device_detection;
      std::map<int32_t, std::shared_ptr<GameController>> _controllers;
};

