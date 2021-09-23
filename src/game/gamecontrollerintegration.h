#pragma once

class JoystickHandler;
class GameController;

class GameControllerIntegration
{
   public:

      GameControllerIntegration();
      virtual ~GameControllerIntegration() = default;

      static int initializeAll();
      static GameControllerIntegration* getInstance(int id);
      static int getCount();

      void initialize(int id = 0);
      GameController* getController();
      void rumble(float intensity, int ms);


private:

      static GameControllerIntegration* createInstance();

      static int __count;
      static GameControllerIntegration* __instances[10];

      GameController* _controller = nullptr;
};

