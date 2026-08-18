#include "gamecontrollerdetection.h"

#include "framework/tools/log.h"

#include <algorithm>
#include <iostream>

// SDL treats added and removed events differently and gives different identifiers for the added and removed events.
// See the SDL documentation below:
//
// Sint32 which;       /**< The joystick device index for the ADDED event, instance id for the REMOVED event */

void GameControllerDetection::start()
{
#ifndef DECEPTUS_VRSFML
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
#endif
}

void GameControllerDetection::stop()
{
#ifndef DECEPTUS_VRSFML
   _stopped = true;
   _thread->join();
#endif
}

void GameControllerDetection::update()
{
#ifdef DECEPTUS_VRSFML
   // No event thread runs on this path: VRSFML owns the SDL event queue and pumps it from the
   // main loop, so a second thread blocking in SDL_WaitEvent would fight it for events -- and
   // taking joystick events out of the queue here would hide them from VRSFML. Comparing the
   // device list against the previous frame needs neither. Without this nothing ever calls the
   // added callback, no controller is ever opened, and every gamepad input is silently ignored,
   // which on a console means no input at all.
   int32_t joystick_count = 0;
   SDL_JoystickID* joystick_ids = SDL_GetJoysticks(&joystick_count);
   if (joystick_ids == nullptr)
   {
      return;
   }

   const std::vector<SDL_JoystickID> current_joystick_ids(joystick_ids, joystick_ids + joystick_count);
   SDL_free(joystick_ids);

   for (const auto joystick_id : current_joystick_ids)
   {
      if (std::ranges::find(_connected_joystick_ids, joystick_id) == _connected_joystick_ids.end())
      {
         Log::Info() << "joystick added, instance id: " << joystick_id;
         _callback_added(joystick_id);
      }
   }

   for (const auto joystick_id : _connected_joystick_ids)
   {
      if (std::ranges::find(current_joystick_ids, joystick_id) == current_joystick_ids.end())
      {
         Log::Info() << "joystick removed, instance id: " << joystick_id;
         _callback_removed(joystick_id);
      }
   }

   _connected_joystick_ids = current_joystick_ids;
#endif
}

int32_t GameControllerDetection::processEvent(const SDL_Event& event)
{
   switch (event.type)
   {
      case SDL_EVENT_JOYSTICK_ADDED:
      {
         Log::Info() << "joystick added, device index: " << event.jdevice.which;
         _callback_added(event.jdevice.which);
         break;
      }
      case SDL_EVENT_JOYSTICK_REMOVED:
      {
         Log::Info() << "joystick removed, device instance id: " << event.jdevice.which;
         _callback_removed(event.jdevice.which);
         break;
      }
      case SDL_EVENT_GAMEPAD_ADDED:
      {
         Log::Info() << "controller device added: " << event.cdevice.which;
         break;
      }
      case SDL_EVENT_GAMEPAD_REMOVED:
      {
         Log::Info() << "controller device removed: " << event.cdevice.which;
         break;
      }
      case SDL_EVENT_GAMEPAD_REMAPPED:
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
