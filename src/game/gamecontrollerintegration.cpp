// header
#include "gamecontrollerintegration.h"

// game
#include "gamejoystickmapping.h"

// joystick
#ifdef TARGET_OS_MAC
#ifdef MAC_OS_X_VERSION_MIN_REQUIRED
#undef MAC_OS_X_VERSION_MIN_REQUIRED
#endif
#define MAC_OS_X_VERSION_MIN_REQUIRED MAC_OS_X_VERSION_10_9
#endif
#include "../framework/joystick/gamecontroller.h"


GameControllerIntegration* GameControllerIntegration::__instances[10];
int GameControllerIntegration::__count = 0;


//-----------------------------------------------------------------------------
GameControllerIntegration::GameControllerIntegration()
{
   _controller = new GameController();
}


//-----------------------------------------------------------------------------
int GameControllerIntegration::initializeAll()
{
   for (auto i = 0; i < 10; i++)
   {
      __instances[i] = nullptr;
   }

   // used for obtaining some information from sdl
   auto tmp = new GameController();
   __count = tmp->getJoystickCount();
   delete tmp;

   for (auto i = 0; i < __count; i++)
   {
      auto gji = createInstance();
      gji->initialize(i);

      __instances[i] = gji;
   }

   return __count;
}


//-----------------------------------------------------------------------------
void GameControllerIntegration::initialize(int id)
{
   // automatically select first in list
   if (_controller->getJoystickCount() > id)
   {
      _controller->setActiveJoystick(id);
   }
}


//-----------------------------------------------------------------------------
GameController* GameControllerIntegration::getController()
{
   return _controller;
}


//-----------------------------------------------------------------------------
void GameControllerIntegration::rumble(float intensity, int ms)
{
   _controller->rumble(intensity, ms);
}


//-----------------------------------------------------------------------------
int GameControllerIntegration::getCount()
{
   return __count;
}


//-----------------------------------------------------------------------------
GameControllerIntegration* GameControllerIntegration::getInstance(int id)
{
   GameControllerIntegration* gji = nullptr;

   if (id >= 0 && id < 10)
      gji = __instances[id];

   return gji;
}


//-----------------------------------------------------------------------------
GameControllerIntegration *GameControllerIntegration::createInstance()
{
   return new GameControllerIntegration();
}

