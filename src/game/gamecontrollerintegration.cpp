// header
#include "gamecontrollerintegration.h"

// game
#include "gamecontrollerdetection.h"
#include "gamejoystickmapping.h"

// joystick
#ifdef TARGET_OS_MAC
#ifdef MAC_OS_X_VERSION_MIN_REQUIRED
#undef MAC_OS_X_VERSION_MIN_REQUIRED
#endif
#define MAC_OS_X_VERSION_MIN_REQUIRED MAC_OS_X_VERSION_10_9
#endif
#include "../framework/joystick/gamecontroller.h"


static constexpr auto instance_count = 10;
static GameControllerIntegration* __instances[instance_count];
std::unique_ptr<GameControllerDetection> GameControllerIntegration::_device_detection;


namespace
{
int32_t count = 0;
}


//-----------------------------------------------------------------------------
GameControllerIntegration::GameControllerIntegration()
{
   _controller = new GameController();
}


//-----------------------------------------------------------------------------
int32_t GameControllerIntegration::initializeAll()
{
   for (auto i = 0; i < instance_count; i++)
   {
      __instances[i] = nullptr;
   }

   // used for obtaining some information from sdl
   auto tmp = new GameController();
   count = tmp->getJoystickCount();
   delete tmp;

   for (auto i = 0; i < count; i++)
   {
      auto gji = new GameControllerIntegration();
      gji->initialize(i);

      __instances[i] = gji;
   }

   _device_detection = std::make_unique<GameControllerDetection>();
   _device_detection->setup();

   return count;
}


//-----------------------------------------------------------------------------
void GameControllerIntegration::initialize(int32_t id)
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
void GameControllerIntegration::rumble(float intensity, int32_t ms)
{
   _controller->rumble(intensity, ms);
}


//-----------------------------------------------------------------------------
int32_t GameControllerIntegration::getCount()
{
   return count;
}


//-----------------------------------------------------------------------------
bool GameControllerIntegration::isControllerConnected()
{
   return getCount() > 0;
}


//-----------------------------------------------------------------------------
GameControllerIntegration* GameControllerIntegration::getInstance(int32_t id)
{
   GameControllerIntegration* gji = nullptr;

   if (id >= 0 && id < instance_count)
   {
      gji = __instances[id];
   }

   return gji;
}

