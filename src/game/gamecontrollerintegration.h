#pragma once

class JoystickHandler;
class GameController;

class GameControllerIntegration
{
   public:

      GameControllerIntegration();
      static int initializeAll();
      static GameControllerIntegration* getInstance(int id);
      virtual ~GameControllerIntegration();
      void initialize(int id = 0);
      GameController* getController();
      void rumble(float intensity, int ms);
      static int getCount();


private:

      static int sCount;
      static GameControllerIntegration* createInstance();
      static GameControllerIntegration* sInstances[10];
      GameController* mController = nullptr;
};

