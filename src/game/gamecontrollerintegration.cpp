// header
#include "gamecontrollerintegration.h"

// game
#include "gamejoystickmapping.h"

// joystick
#ifdef Q_OS_MAC
#ifdef MAC_OS_X_VERSION_MIN_REQUIRED
#undef MAC_OS_X_VERSION_MIN_REQUIRED
#endif
#define MAC_OS_X_VERSION_MIN_REQUIRED MAC_OS_X_VERSION_10_9
#endif
#include "../joystick/gamecontroller.h"


GameControllerIntegration* GameControllerIntegration::sInstances[10];
int GameControllerIntegration::sCount = 0;


//-----------------------------------------------------------------------------
GameControllerIntegration::GameControllerIntegration()
{
   mController = new GameController();
}


//-----------------------------------------------------------------------------
int GameControllerIntegration::initializeAll()
{
   for (auto i = 0; i < 10; i++)
      sInstances[i] = nullptr;

   // used for obtaining some information from sdl
   auto tmp = new GameController();
   sCount = tmp->getJoystickCount();
   delete tmp;

   for (auto i = 0; i < sCount; i++)
   {
      auto gji = createInstance();
      gji->initialize(i);

      sInstances[i] = gji;
   }

   return sCount;
}


//-----------------------------------------------------------------------------
GameControllerIntegration::~GameControllerIntegration()
{
}


//-----------------------------------------------------------------------------
void GameControllerIntegration::initialize(int id)
{
   // automatically select first in list
   if (mController->getJoystickCount() > id)
   {
      mController->setActiveJoystick(id);
   }
}


//-----------------------------------------------------------------------------
GameController* GameControllerIntegration::getController()
{
   return mController;
}


//-----------------------------------------------------------------------------
void GameControllerIntegration::rumble(float intensity, int ms)
{
   mController->rumble(intensity, ms);
}


//-----------------------------------------------------------------------------
int GameControllerIntegration::getCount()
{
   return sCount;
}


//-----------------------------------------------------------------------------
GameControllerIntegration* GameControllerIntegration::getInstance(int id)
{
   GameControllerIntegration* gji = nullptr;

   if (id >= 0 && id < 10)
      gji = sInstances[id];

   return gji;
}


//-----------------------------------------------------------------------------
GameControllerIntegration *GameControllerIntegration::createInstance()
{
   return new GameControllerIntegration();
}

