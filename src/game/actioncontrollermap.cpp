#include "actioncontrollermap.h"


// maybe this whole class is pointless and should be deleted.


ActionControllerMap::ActionControllerMap()
{
  mButtonMap[SDL_CONTROLLER_BUTTON_A            ] = SdlControllerButtonA            ;
  mButtonMap[SDL_CONTROLLER_BUTTON_B            ] = SdlControllerButtonB            ;
  mButtonMap[SDL_CONTROLLER_BUTTON_X            ] = SdlControllerButtonX            ;
  mButtonMap[SDL_CONTROLLER_BUTTON_Y            ] = SdlControllerButtonY            ;
  mButtonMap[SDL_CONTROLLER_BUTTON_BACK         ] = SdlControllerButtonBack         ;
  mButtonMap[SDL_CONTROLLER_BUTTON_GUIDE        ] = SdlControllerButtonGuide        ;
  mButtonMap[SDL_CONTROLLER_BUTTON_START        ] = SdlControllerButtonStart        ;
  mButtonMap[SDL_CONTROLLER_BUTTON_LEFTSTICK    ] = SdlControllerButtonLeftStick    ;
  mButtonMap[SDL_CONTROLLER_BUTTON_RIGHTSTICK   ] = SdlControllerButtonRightStick   ;
  mButtonMap[SDL_CONTROLLER_BUTTON_LEFTSHOULDER ] = SdlControllerButtonLeftShoulder ;
  mButtonMap[SDL_CONTROLLER_BUTTON_RIGHTSHOULDER] = SdlControllerButtonRightShoulder;
  mButtonMap[SDL_CONTROLLER_BUTTON_DPAD_UP      ] = SdlControllerButtonDpadUp       ;
  mButtonMap[SDL_CONTROLLER_BUTTON_DPAD_DOWN    ] = SdlControllerButtonDpadDown     ;
  mButtonMap[SDL_CONTROLLER_BUTTON_DPAD_LEFT    ] = SdlControllerButtonDpadLeft     ;
  mButtonMap[SDL_CONTROLLER_BUTTON_DPAD_RIGHT   ] = SdlControllerButtonDpadRight    ;
  mButtonMap[SDL_CONTROLLER_BUTTON_INVALID      ] = SdlControllerButtonNone         ;
}


PlayerAction ActionControllerMap::getAction(
  SDL_GameControllerButton button1,
  SDL_GameControllerButton button2,
  SDL_GameControllerButton button3,
  SDL_GameControllerButton button4
)
{
  auto buttons =
      mButtonMap[button1]
    | mButtonMap[button2]
    | mButtonMap[button3]
    | mButtonMap[button4];

  auto action = PlayerAction::None;
  if (buttons & SdlControllerButtonA)
  {
    if (buttons & SdlControllerButtonDpadDown)
    {
      action = PlayerAction::DropPlatform;
    }
    else
    {
      action = PlayerAction::Jump;
    }
  }
  else if (buttons & SdlControllerButtonB)
  {
    action = PlayerAction::Action;
  }
  else if (buttons & SdlControllerButtonX)
  {
    action = PlayerAction::Shoot;
  }
  else if (buttons & SdlControllerButtonY)
  {
    action = PlayerAction::Inventory;
  }
  else if (buttons & SdlControllerButtonDpadUp)
  {
    action = PlayerAction::EnterDoor;
  }
  else if (buttons & SdlControllerButtonDpadLeft)
  {
    action = PlayerAction::MoveLeft;
  }
  else if (buttons & SdlControllerButtonDpadRight)
  {
    action = PlayerAction::MoveRight;
  }
  else if (buttons & SdlControllerButtonRightShoulder)
  {
    action = PlayerAction::LookAround;
  }

  return action;
}

