#include "gamecontrollerdetection.h"

#include "framework/tools/log.h"

#include <iostream>


void GameControllerDetection::setup()
{
   _thread = std::make_unique<std::thread>(
      [this](){
         SDL_Event event;
         while (!_stopped && SDL_WaitEvent(&event)) {
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
         Log::Info() << "joystick added: " << event.jdevice.which;
         break;
      }
      case SDL_JOYDEVICEREMOVED:
      {
         Log::Info() << "joystick removed: " << event.jdevice.which;
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
   }
   return 0;
}
