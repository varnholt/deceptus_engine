#include "playerinput.h"


void PlayerInput::update()
{
}


bool PlayerInput::isKeyboardUsed() const
{
   return (_input_type == InputType::Keyboard);
}


bool PlayerInput::isControllerUsed() const
{
   return (_input_type == InputType::Controller);
}
