#include "gamecontrollerintegration.h"

// joystick
#ifdef TARGET_OS_MAC
#ifdef MAC_OS_X_VERSION_MIN_REQUIRED
#undef MAC_OS_X_VERSION_MIN_REQUIRED
#endif
#define MAC_OS_X_VERSION_MIN_REQUIRED MAC_OS_X_VERSION_10_9
#endif

#include "framework/joystick/gamecontroller.h"
#include "framework/tools/log.h"
#include "gamecontrollerdetection.h"

#include <iostream>

int32_t GameControllerIntegration::_selected_controller_id = 0;

void GameControllerIntegration::initialize()
{
   SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC);

   auto res = SDL_GameControllerAddMappingsFromFile("data/joystick/gamecontrollerdb.txt");
   if (res == -1)
   {
      Log::Error() << "error loading game controller database";
   }

   _device_detection = std::make_unique<GameControllerDetection>();
   _device_detection->setCallbackAdded([this](int32_t id) { add(id); });
   _device_detection->setCallbackRemoved([this](int32_t id) { remove(id); });
   _device_detection->start();
}

GameControllerIntegration::~GameControllerIntegration()
{
   (void)_device_detection.release();
   SDL_QuitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC);
}

void GameControllerIntegration::update()
{
   const std::lock_guard<std::mutex> lock(_device_changed_mutex);
   for (const auto& cb : _device_changed_callbacks)
   {
      cb();
   }

   _device_changed_callbacks.clear();
}

void GameControllerIntegration::add(int32_t id)
{
   const std::lock_guard<std::mutex> lock(_device_changed_mutex);
   _device_changed_callbacks.emplace_back(
      [this, id]()
      {
         auto controller = std::make_shared<GameController>();
         controller->activate(id);
         const auto controller_id = SDL_JoystickGetDeviceInstanceID(id);
         _controllers[controller_id] = controller;
         _selected_controller_id = controller_id;

         for (const auto& cb : _device_added_callbacks)
         {
            cb(controller_id);
         }
      }
   );
}

void GameControllerIntegration::remove(int32_t id)
{
   const std::lock_guard<std::mutex> lock(_device_changed_mutex);
   _device_changed_callbacks.emplace_back(
      [this, id]()
      {
         _controllers.erase(id);

         for (const auto& cb : _device_removed_callbacks)
         {
            cb(id);
         }
      }
   );
}

const std::shared_ptr<GameController>& GameControllerIntegration::getController(int32_t controller_id) const
{
   return _controllers.at(controller_id);
}

void GameControllerIntegration::addDeviceAddedCallback(const DeviceAddedCallback& callback)
{
   _device_added_callbacks.push_back(callback);

   // call added for all devices we already have
   for (auto& [k, v] : _controllers)
   {
      callback(k);
   }
}

void GameControllerIntegration::addDeviceRemovedCallback(const DeviceAddedCallback& callback)
{
   _device_removed_callbacks.push_back(callback);
}

size_t GameControllerIntegration::getCount() const
{
   return _controllers.size();
}

bool GameControllerIntegration::isControllerConnected() const
{
   return !_controllers.empty();
}

GameControllerIntegration& GameControllerIntegration::getInstance()
{
   static GameControllerIntegration __gci;
   return __gci;
}
