#pragma once

#include <cstdint>

class JoystickHandler;
class GameController;

class GameControllerIntegration
{
   public:

      GameControllerIntegration();
      virtual ~GameControllerIntegration() = default;

      static int32_t initializeAll();
      static GameControllerIntegration* getInstance(int32_t id);
      static int32_t getCount();
      static bool controllerConnected();

      void initialize(int32_t id = 0);
      GameController* getController();
      void rumble(float intensity, int32_t ms);


private:

      static GameControllerIntegration* createInstance();

      static int32_t __count;
      static GameControllerIntegration* __instances[10];

      GameController* _controller = nullptr;
};

