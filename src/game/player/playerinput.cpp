#include "playerinput.h"


void PlayerInput::update(InputType input_type)
{
   _input_type = input_type;
}


bool PlayerInput::isKeyboardUsed() const
{
   return (_input_type == InputType::Keyboard);
}


bool PlayerInput::isControllerUsed() const
{
   return (_input_type == InputType::Controller);
}
