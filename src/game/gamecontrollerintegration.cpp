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


//-----------------------------------------------------------------------------
void GameControllerIntegration::initialize()
{
   _device_detection = std::make_unique<GameControllerDetection>();
   _device_detection->setCallbackAdded([this](int32_t id){add(id);});
   _device_detection->setCallbackRemoved([this](int32_t id){remove(id);});
   _device_detection->start();
}


//-----------------------------------------------------------------------------
void GameControllerIntegration::add(int32_t id)
{
   auto controller = std::make_shared<GameController>();
   controller->setActiveJoystick(id);
   _controllers[id] = controller;
}


//-----------------------------------------------------------------------------
void GameControllerIntegration::remove(int32_t id)
{

}


//-----------------------------------------------------------------------------
std::shared_ptr<GameController>& GameControllerIntegration::getController(int32_t controller_id)
{
   return _controllers[controller_id];
}


//-----------------------------------------------------------------------------
void GameControllerIntegration::rumble(float intensity, int32_t ms, int32_t controller_id)
{
   _controllers[controller_id]->rumble(intensity, ms);
}


//-----------------------------------------------------------------------------
int32_t GameControllerIntegration::getCount() const
{
   return _controllers.size();
}


//-----------------------------------------------------------------------------
bool GameControllerIntegration::isControllerConnected() const
{
   return !_controllers.empty();
}


//-----------------------------------------------------------------------------
GameControllerIntegration& GameControllerIntegration::getInstance()
{
   static GameControllerIntegration __gci;
   return __gci;
}

