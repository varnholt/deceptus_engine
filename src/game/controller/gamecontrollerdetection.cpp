#include "gamecontrollerdetection.h"

#include "framework/tools/log.h"

#include <iostream>

// SDL treats added and removed events differently and gives different identifiers for the added and removed events.
// See the SDL documentation below:
//
// Sint32 which;       /**< The joystick device index for the ADDED event, instance id for the REMOVED event */

void GameControllerDetection::start()
{
   _thread = std::make_unique<std::thread>(
      [this]()
      {
         SDL_Event event;
         while (!_stopped && SDL_WaitEvent(&event))
         {
            processEvent(event);
         }
      }
   );
}

void GameControllerDetection::stop()
{
   _stopped = true;
   _thread->join();
}

int32_t GameControllerDetection::processEvent(const SDL_Event& event)
{
   switch (event.type)
   {
      case SDL_JOYDEVICEADDED:
      {
         Log::Info() << "joystick added, device index: " << event.jdevice.which;
         _callback_added(event.jdevice.which);
         break;
      }
      case SDL_JOYDEVICEREMOVED:
      {
         Log::Info() << "joystick removed, device instance id: " << event.jdevice.which;
         _callback_removed(event.jdevice.which);
         break;
      }
      case SDL_CONTROLLERDEVICEADDED:
      {
         Log::Info() << "controller device added: " << event.cdevice.which;
         break;
      }
      case SDL_CONTROLLERDEVICEREMOVED:
      {
         Log::Info() << "controller device removed: " << event.cdevice.which;
         break;
      }
      case SDL_CONTROLLERDEVICEREMAPPED:
      {
         Log::Info() << "controller device remapped: " << event.cdevice.which;
         break;
      }
      default:
      {
         break;
      }
   }
   return 0;
}

void GameControllerDetection::setCallbackRemoved(const RemovedCallback& callback_added)
{
   _callback_removed = callback_added;
}

void GameControllerDetection::setCallbackAdded(const AddedCallback& callback_removed)
{
   _callback_added = callback_removed;
}
