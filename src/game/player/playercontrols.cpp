#include "playercontrols.h"

#include "constants.h"
#include "framework/joystick/gamecontroller.h"
#include "gamecontrollerintegration.h"
#include "tweaks.h"

//----------------------------------------------------------------------------------------------------------------------
void PlayerControls::update(const sf::Time& /*dt*/)
{
   // store where the player has received input from last time
   updatePlayerInput();

   setWasMoving(isMovingHorizontally());
   setWasMovingLeft(isMovingLeft());
   setWasMovingRight(isMovingRight());
}

//----------------------------------------------------------------------------------------------------------------------
void PlayerControls::addKeypressedCallback(const KeypressedCallback& callback)
{
   _keypressed_callbacks.push_back(callback);
}

//----------------------------------------------------------------------------------------------------------------------
bool PlayerControls::hasFlag(int32_t flag) const
{
   return _keys_pressed & flag;
}

//----------------------------------------------------------------------------------------------------------------------
void PlayerControls::forceSync()
{
   if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space))
   {
      _keys_pressed |= KeyPressedJump;
   }

   if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift))
   {
      _keys_pressed |= KeyPressedLook;
   }

   if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
   {
      _keys_pressed |= KeyPressedUp;
   }

   if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
   {
      _keys_pressed |= KeyPressedDown;
   }

   if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
   {
      _keys_pressed |= KeyPressedLeft;
   }

   if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
   {
      _keys_pressed |= KeyPressedRight;
   }

   if (sf::Keyboard::isKeyPressed(sf::Keyboard::LAlt))
   {
      _keys_pressed |= KeyPressedRun;
   }

   if (sf::Keyboard::isKeyPressed(sf::Keyboard::LControl))
   {
      _keys_pressed |= KeyPressedFire;
   }
}

//----------------------------------------------------------------------------------------------------------------------
void PlayerControls::keyboardKeyPressed(sf::Keyboard::Key key)
{
   _player_input.update(PlayerInput::InputType::Keyboard);

   if (key == sf::Keyboard::Space)
   {
      _keys_pressed |= KeyPressedJump;
   }
   else if (key == sf::Keyboard::LShift)
   {
      _keys_pressed |= KeyPressedLook;
   }
   else if (key == sf::Keyboard::Up)
   {
      _keys_pressed |= KeyPressedUp;
   }
   else if (key == sf::Keyboard::Down)
   {
      _keys_pressed |= KeyPressedDown;
   }
   else if (key == sf::Keyboard::Left)
   {
      _keys_pressed |= KeyPressedLeft;
   }
   else if (key == sf::Keyboard::Right)
   {
      _keys_pressed |= KeyPressedRight;
   }
   else if (key == sf::Keyboard::LAlt)
   {
      _keys_pressed |= KeyPressedRun;
   }
   else if (key == sf::Keyboard::LControl)
   {
      _keys_pressed |= KeyPressedFire;
   }

   for (auto& callback : _keypressed_callbacks)
   {
      callback(key);
   }
}

//----------------------------------------------------------------------------------------------------------------------
void PlayerControls::keyboardKeyReleased(sf::Keyboard::Key key)
{
   _player_input.update(PlayerInput::InputType::Keyboard);

   if (key == sf::Keyboard::LShift)
   {
      _keys_pressed &= ~KeyPressedLook;
   }
   else if (key == sf::Keyboard::Up)
   {
      _keys_pressed &= ~KeyPressedUp;
   }
   else if (key == sf::Keyboard::Down)
   {
      _keys_pressed &= ~KeyPressedDown;
   }
   else if (key == sf::Keyboard::Left)
   {
      _keys_pressed &= ~KeyPressedLeft;
   }
   else if (key == sf::Keyboard::Right)
   {
      _keys_pressed &= ~KeyPressedRight;
   }
   else if (key == sf::Keyboard::Space)
   {
      _keys_pressed &= ~KeyPressedJump;
   }
   else if (key == sf::Keyboard::LAlt)
   {
      _keys_pressed &= ~KeyPressedRun;
   }
   else if (key == sf::Keyboard::LControl)
   {
      _keys_pressed &= ~KeyPressedFire;
   }
}

//----------------------------------------------------------------------------------------------------------------------
bool PlayerControls::isLookingAround() const
{
   if (_keys_pressed & KeyPressedLook)
   {
      return true;
   }

   if (GameControllerIntegration::getInstance().isControllerConnected())
   {
      return isControllerButtonPressed(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
   }

   return false;
}

//----------------------------------------------------------------------------------------------------------------------
bool PlayerControls::isControllerButtonPressed(int button_enum) const
{
   if (!GameControllerIntegration::getInstance().isControllerConnected())
   {
      return false;
   }

   _joystick_info.getButtonValues();

   // does not need to be mapped
   // auto button_id = gji->getController()->getButtonId(static_cast<SDL_GameControllerButton>(button_enum));
   // pressed = (_joystick_info.getButtonValues()[static_cast<size_t>(button_id)]);

   const auto pressed = (_joystick_info.getButtonValues()[static_cast<size_t>(button_enum)]);
   return pressed;
}

//----------------------------------------------------------------------------------------------------------------------
bool PlayerControls::isFireButtonPressed() const
{
   if (_keys_pressed & KeyPressedFire)
   {
      return true;
   }

   if (GameControllerIntegration::getInstance().isControllerConnected())
   {
      return isControllerButtonPressed(SDL_CONTROLLER_BUTTON_X);
   }

   return false;
}

//----------------------------------------------------------------------------------------------------------------------
bool PlayerControls::isJumpButtonPressed() const
{
   if (_keys_pressed & KeyPressedJump)
   {
      return true;
   }

   if (GameControllerIntegration::getInstance().isControllerConnected())
   {
      return isControllerButtonPressed(SDL_CONTROLLER_BUTTON_A);
   }

   return false;
}

//----------------------------------------------------------------------------------------------------------------------
bool PlayerControls::isUpButtonPressed() const
{
   if (_keys_pressed & KeyPressedUp)
   {
      return true;
   }

   if (GameControllerIntegration::getInstance().isControllerConnected())
   {
      return isControllerButtonPressed(SDL_CONTROLLER_BUTTON_DPAD_UP);
   }

   return false;
}

//----------------------------------------------------------------------------------------------------------------------
bool PlayerControls::isDownButtonPressed() const
{
   if (_keys_pressed & KeyPressedDown)
   {
      return true;
   }

   if (GameControllerIntegration::getInstance().isControllerConnected())
   {
      return isControllerButtonPressed(SDL_CONTROLLER_BUTTON_DPAD_DOWN);
   }

   return false;
}

//----------------------------------------------------------------------------------------------------------------------
bool PlayerControls::isDroppingDown() const
{
   return isJumpButtonPressed() && isMovingDown();
}

//----------------------------------------------------------------------------------------------------------------------
bool PlayerControls::isMovingLeft() const
{
   // controller input
   if (GameControllerIntegration::getInstance().isControllerConnected())
   {
      const auto& axis_values = _joystick_info.getAxisValues();
      const auto axis_left_x = GameControllerIntegration::getInstance().getController()->getAxisIndex(SDL_CONTROLLER_AXIS_LEFTX);
      auto xl = axis_values[static_cast<size_t>(axis_left_x)] / 32767.0f;
      const auto hat_value = _joystick_info.getHatValues().at(0);
      const auto dpad_left_pressed = hat_value & SDL_HAT_LEFT;
      const auto dpad_right_pressed = hat_value & SDL_HAT_RIGHT;

      if (dpad_left_pressed)
      {
         xl = -1.0f;
      }
      else if (dpad_right_pressed)
      {
         xl = 1.0f;
      }

      if (fabs(xl) > 0.3f)
      {
         if (xl < 0.0f)
         {
            return true;
         }
      }
   }

   // keyboard input
   if (_keys_pressed & KeyPressedLeft)
   {
      return true;
   }

   return false;
}

//----------------------------------------------------------------------------------------------------------------------
bool PlayerControls::isMovingDown() const
{
   // controller input
   if (GameControllerIntegration::getInstance().isControllerConnected())
   {
      const auto& axis_values = _joystick_info.getAxisValues();
      const auto axis_left_y = GameControllerIntegration::getInstance().getController()->getAxisIndex(SDL_CONTROLLER_AXIS_LEFTY);
      auto y1 = axis_values[static_cast<size_t>(axis_left_y)] / 32767.0f;
      const auto hat_value = _joystick_info.getHatValues().at(0);
      const auto dpad_down_pressed = hat_value & SDL_HAT_DOWN;
      const auto dpad_right_pressed = hat_value & SDL_HAT_UP;

      if (dpad_down_pressed)
      {
         y1 = 1.0f;
      }
      else if (dpad_right_pressed)
      {
         y1 = 1.0f;
      }

      if (fabs(y1) > 0.3f)
      {
         if (y1 > 0.0f)
         {
            return true;
         }
      }
   }

   // keyboard input
   if (_keys_pressed & KeyPressedDown)
   {
      return true;
   }

   return false;
}

//----------------------------------------------------------------------------------------------------------------------
bool PlayerControls::isMovingRight() const
{
   // controller input
   if (GameControllerIntegration::getInstance().isControllerConnected())
   {
      const auto& axis_values = _joystick_info.getAxisValues();
      const auto axis_left_x = GameControllerIntegration::getInstance().getController()->getAxisIndex(SDL_CONTROLLER_AXIS_LEFTX);
      auto xl = axis_values[static_cast<size_t>(axis_left_x)] / 32767.0f;
      const auto hat_value = _joystick_info.getHatValues().at(0);
      const auto dpad_left_pressed = hat_value & SDL_HAT_LEFT;
      const auto dpad_right_pressed = hat_value & SDL_HAT_RIGHT;

      if (dpad_left_pressed)
      {
         xl = -1.0f;
      }
      else if (dpad_right_pressed)
      {
         xl = 1.0f;
      }

      if (fabs(xl) > 0.3f)
      {
         if (xl > 0.0f)
         {
            return true;
         }
      }
   }

   // keyboard input
   if (_keys_pressed & KeyPressedRight)
   {
      return true;
   }

   return false;
}

//----------------------------------------------------------------------------------------------------------------------
bool PlayerControls::isMovingHorizontally() const
{
   return isMovingLeft() || isMovingRight();
}

//----------------------------------------------------------------------------------------------------------------------
int PlayerControls::getKeysPressed() const
{
   return _keys_pressed;
}

//----------------------------------------------------------------------------------------------------------------------
void PlayerControls::setKeysPressed(int32_t keysPressed)
{
   _keys_pressed = keysPressed;
}

//----------------------------------------------------------------------------------------------------------------------
const GameControllerInfo& PlayerControls::getJoystickInfo() const
{
   return _joystick_info;
}

//----------------------------------------------------------------------------------------------------------------------
void PlayerControls::setJoystickInfo(const GameControllerInfo& joystick_info)
{
   _joystick_info = joystick_info;
}

//----------------------------------------------------------------------------------------------------------------------
bool PlayerControls::wasMoving() const
{
   return _was_moving;
}

//----------------------------------------------------------------------------------------------------------------------
void PlayerControls::setWasMoving(bool was_moving)
{
   _was_moving = was_moving;
}

//----------------------------------------------------------------------------------------------------------------------
bool PlayerControls::wasMovingLeft() const
{
   return _was_moving_left;
}

//----------------------------------------------------------------------------------------------------------------------
void PlayerControls::setWasMovingLeft(bool was_moving_left)
{
   _was_moving_left = was_moving_left;
}

//----------------------------------------------------------------------------------------------------------------------
bool PlayerControls::wasMovingRight() const
{
   return _was_moving_right;
}

//----------------------------------------------------------------------------------------------------------------------
void PlayerControls::setWasMovingRight(bool was_moving_right)
{
   _was_moving_right = was_moving_right;
}

//----------------------------------------------------------------------------------------------------------------------
bool PlayerControls::changedToIdle() const
{
   return wasMoving() && !isMovingHorizontally();
}

//----------------------------------------------------------------------------------------------------------------------
bool PlayerControls::changedToMoving() const
{
   return !wasMoving() && isMovingHorizontally();
}

//----------------------------------------------------------------------------------------------------------------------
PlayerControls::Orientation PlayerControls::updateOrientation()
{
   Orientation orientation = Orientation::Undefined;

   // controller input
   if (GameControllerIntegration::getInstance().isControllerConnected())
   {
      const auto& axis_values = getJoystickInfo().getAxisValues();
      const auto axis_left_x = GameControllerIntegration::getInstance().getController()->getAxisIndex(SDL_CONTROLLER_AXIS_LEFTX);
      auto xl = axis_values[static_cast<size_t>(axis_left_x)] / 32767.0f;
      const auto hat_value = getJoystickInfo().getHatValues().at(0);
      const auto dpad_left_pressed = hat_value & SDL_HAT_LEFT;
      const auto dpad_right_pressed = hat_value & SDL_HAT_RIGHT;

      if (dpad_left_pressed)
      {
         xl = -1.0f;
      }
      else if (dpad_right_pressed)
      {
         xl = 1.0f;
      }

      if (fabs(xl) > 0.3f)
      {
         if (xl < 0.0f)
         {
            orientation = Orientation::Left;
         }
         else
         {
            orientation = Orientation::Right;
         }
      }
   }

   // keyboard input
   if (hasFlag(KeyPressedLeft))
   {
      orientation = Orientation::Left;
   }

   if (hasFlag(KeyPressedRight))
   {
      orientation = Orientation::Right;
   }

   // while the orientation is locked, just return the last orientation the player had
   if (std::chrono::high_resolution_clock::now() < _unlock_orientation_time_point)
   {
      if (orientation != Orientation::Undefined)
      {
         _last_requested_orientation = orientation;
      }

      orientation = _locked_orientation;
   }
   else
   {
      // if the user presses a key or button while the orientation is locked, we want to apply that
      // button as soon as the orientation is unlocked again.
      if (_last_requested_orientation != Orientation::Undefined)
      {
         orientation = _last_requested_orientation;
         _last_requested_orientation = Orientation::Undefined;
      }
   }

   return orientation;
}

//----------------------------------------------------------------------------------------------------------------------
bool PlayerControls::isBendDownActive() const
{
   auto down_pressed = false;

   if (GameControllerIntegration::getInstance().isControllerConnected())
   {
      const auto& joystick_info = getJoystickInfo();
      const auto& axis_values = joystick_info.getAxisValues();

      const auto axis_lefy_y = GameControllerIntegration::getInstance().getController()->getAxisIndex(SDL_CONTROLLER_AXIS_LEFTY);
      auto yl = axis_values[static_cast<size_t>(axis_lefy_y)] / 32767.0f;
      const auto& hat_value = joystick_info.getHatValues().at(0);
      auto dpad_down_pressed = hat_value & SDL_HAT_DOWN;

      if (dpad_down_pressed)
      {
         yl = 1.0f;
      }

      if (fabs(yl) > Tweaks::instance()._bend_down_threshold)
      {
         if (yl > 0.0f)
         {
            down_pressed = true;
         }
      }
   }

   // keyboard input
   if (hasFlag(KeyPressedDown))
   {
      down_pressed = true;
   }

   return down_pressed;
}

bool PlayerControls::isControllerUsedLast() const
{
   return _player_input.isControllerUsed();
}

void PlayerControls::lockOrientation(std::chrono::milliseconds interval)
{
   _locked_orientation = updateOrientation();

   const auto now = std::chrono::high_resolution_clock::now();
   _unlock_orientation_time_point = now + interval;
}

void PlayerControls::updatePlayerInput()
{
   // keyboard input is already evaluated from keyboard events

   // evaluate controller input
   if (!GameControllerIntegration::getInstance().isControllerConnected())
   {
      _player_input.update(PlayerInput::InputType::Keyboard);
      return;
   }

   const auto& axis_values = _joystick_info.getAxisValues();
   const auto axis_left_x = GameControllerIntegration::getInstance().getController()->getAxisIndex(SDL_CONTROLLER_AXIS_LEFTX);
   const auto xl = axis_values[static_cast<size_t>(axis_left_x)] / 32767.0f;
   const auto hat_value = _joystick_info.getHatValues().at(0);

   const auto dpad_left_pressed = hat_value & SDL_HAT_LEFT;
   const auto dpad_right_pressed = hat_value & SDL_HAT_RIGHT;

   if (dpad_left_pressed)
   {
      _player_input.update(PlayerInput::InputType::Controller);
   }

   if (dpad_right_pressed)
   {
      _player_input.update(PlayerInput::InputType::Controller);
   }

   if (fabs(xl) > 0.3f)
   {
      _player_input.update(PlayerInput::InputType::Controller);
   }
}
