#include "playercontrols.h"

#include "framework/joystick/gamecontroller.h"
#include "framework/tools/log.h"
#include "game/config/tweaks.h"
#include "game/controller/gamecontrollerintegration.h"
#include "game/player/playercontrolstate.h"

void PlayerControls::update(const sf::Time& dt)
{
   updateLockedKeys(dt);

   // store where the player has received input from last time
   updatePlayerInput();

   setWasMoving(isMovingHorizontally());
   setWasMovingLeft(isMovingLeft());
   setWasMovingRight(isMovingRight());
}

void PlayerControls::addKeypressedCallback(const KeypressedCallback& callback)
{
   _keypressed_callbacks.push_back(callback);
}

bool PlayerControls::hasFlag(KeyPressed flag) const
{
   // check if button state is locked
   const auto it = readLockedState(flag);
   if (it != _locked_keys.end())
   {
      return it->second.asBool();
   }

   return _keys_pressed & flag;
}

void PlayerControls::forceSync()
{
   if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space))
   {
      _keys_pressed |= KeyPressedJump;
   }

   // if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift))
   // {
   //    _keys_pressed |= KeyPressedLook;
   // }

   if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))
   {
      _keys_pressed |= KeyPressedUp;
   }

   if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))
   {
      _keys_pressed |= KeyPressedDown;
   }

   if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
   {
      _keys_pressed |= KeyPressedLeft;
   }

   if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
   {
      _keys_pressed |= KeyPressedRight;
   }

   if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl))
   {
      _keys_pressed |= KeyPressedSlot1;
   }

   if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LAlt))
   {
      _keys_pressed |= KeyPressedSlot2;
   }

   if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Enter))
   {
      _keys_pressed |= KeyPressedAction;
   }
}

void PlayerControls::keyboardKeyPressed(sf::Keyboard::Key key)
{
   _player_input.update(PlayerInput::InputType::Keyboard);

   if (key == sf::Keyboard::Key::Space)
   {
      _keys_pressed |= KeyPressedJump;
   }
   // else if (key == sf::Keyboard::Key::LShift)
   // {
   //    _keys_pressed |= KeyPressedLook;
   // }
   else if (key == sf::Keyboard::Key::Up)
   {
      _keys_pressed |= KeyPressedUp;
   }
   else if (key == sf::Keyboard::Key::Down)
   {
      _keys_pressed |= KeyPressedDown;
   }
   else if (key == sf::Keyboard::Key::Left)
   {
      _keys_pressed |= KeyPressedLeft;
   }
   else if (key == sf::Keyboard::Key::Right)
   {
      _keys_pressed |= KeyPressedRight;
   }
   else if (key == sf::Keyboard::Key::LControl)
   {
      _keys_pressed |= KeyPressedSlot1;
   }
   else if (key == sf::Keyboard::Key::LAlt)
   {
      _keys_pressed |= KeyPressedSlot2;
   }
   else if (key == sf::Keyboard::Key::Enter)
   {
      _keys_pressed |= KeyPressedAction;
   }

   for (const auto& callback : _keypressed_callbacks)
   {
      callback(key);
   }
}

void PlayerControls::keyboardKeyReleased(sf::Keyboard::Key key)
{
   _player_input.update(PlayerInput::InputType::Keyboard);

   // if (key == sf::Keyboard::Key::LShift)
   // {
   //    _keys_pressed &= ~KeyPressedLook;
   // }
   /*else */ if (key == sf::Keyboard::Key::Up)
   {
      _keys_pressed &= ~KeyPressedUp;
   }
   else if (key == sf::Keyboard::Key::Down)
   {
      _keys_pressed &= ~KeyPressedDown;
   }
   else if (key == sf::Keyboard::Key::Left)
   {
      _keys_pressed &= ~KeyPressedLeft;
   }
   else if (key == sf::Keyboard::Key::Right)
   {
      _keys_pressed &= ~KeyPressedRight;
   }
   else if (key == sf::Keyboard::Key::Space)
   {
      _keys_pressed &= ~KeyPressedJump;
   }
   else if (key == sf::Keyboard::Key::LControl)
   {
      _keys_pressed &= ~KeyPressedSlot1;
   }
   else if (key == sf::Keyboard::Key::LAlt)
   {
      _keys_pressed &= ~KeyPressedSlot2;
   }
   else if (key == sf::Keyboard::Key::Enter)
   {
      _keys_pressed &= ~KeyPressedAction;
   }
}

bool PlayerControls::isCpanControlActive() const
{
   // check if look key is pressed
   if (_keys_pressed & KeyPressedLook)
   {
      return true;
   }

   // check if right analogue stick is beyond the configured tolerance
   if (GameControllerIntegration::getInstance().isControllerConnected())
   {
      const auto& axis_values = _joystick_info.getAxisValues();
      const auto x_axis = GameControllerIntegration::getInstance().getController()->getAxisIndex(SDL_GAMEPAD_AXIS_RIGHTX);
      const auto y_axis = GameControllerIntegration::getInstance().getController()->getAxisIndex(SDL_GAMEPAD_AXIS_RIGHTY);
      const auto x_normalized = axis_values[static_cast<uint32_t>(x_axis)] / 32767.0f;
      const auto y_normalized = axis_values[static_cast<uint32_t>(y_axis)] / 32767.0f;
      const auto tolerance_x = Tweaks::instance()._cpan_tolerance_x;
      const auto tolerance_y = Tweaks::instance()._cpan_tolerance_y;
      return (fabs(x_normalized) > tolerance_x || fabs(y_normalized) > tolerance_y);
   }

   return false;
}

bool PlayerControls::isControllerButtonPressed(int32_t button_enum) const
{
   if (!PlayerControlState::checkState())
   {
      return false;
   }

   if (!GameControllerIntegration::getInstance().isControllerConnected())
   {
      return false;
   }

   _joystick_info.getButtonValues();

   const auto pressed = (_joystick_info.getButtonValues()[static_cast<size_t>(button_enum)]);
   return pressed;
}

bool PlayerControls::isButtonXPressed() const
{
   if (!PlayerControlState::checkState())
   {
      return false;
   }

   if (_keys_pressed & KeyPressedSlot1)
   {
      return true;
   }

   if (GameControllerIntegration::getInstance().isControllerConnected())
   {
      return isControllerButtonPressed(SDL_GAMEPAD_BUTTON_WEST);
   }

   return false;
}

bool PlayerControls::isButtonYPressed() const
{
   if (!PlayerControlState::checkState())
   {
      return false;
   }

   if (_keys_pressed & KeyPressedSlot2)
   {
      return true;
   }

   if (GameControllerIntegration::getInstance().isControllerConnected())
   {
      return isControllerButtonPressed(SDL_GAMEPAD_BUTTON_NORTH);
   }

   return false;
}

bool PlayerControls::isButtonAPressed() const
{
   if (!PlayerControlState::checkState())
   {
      return false;
   }

   if (_keys_pressed & KeyPressedJump)
   {
      return true;
   }

   if (GameControllerIntegration::getInstance().isControllerConnected())
   {
      return isControllerButtonPressed(SDL_GAMEPAD_BUTTON_SOUTH);
   }

   return false;
}

bool PlayerControls::isButtonBPressed() const
{
   if (!PlayerControlState::checkState())
   {
      return false;
   }

   if (_keys_pressed & KeyPressedAction)
   {
      return true;
   }

   if (GameControllerIntegration::getInstance().isControllerConnected())
   {
      return isControllerButtonPressed(SDL_GAMEPAD_BUTTON_EAST);
   }

   return false;
}

bool PlayerControls::isUpButtonPressed() const
{
   if (!PlayerControlState::checkState())
   {
      return false;
   }

   if (_keys_pressed & KeyPressedUp)
   {
      return true;
   }

   if (GameControllerIntegration::getInstance().isControllerConnected())
   {
      return isControllerButtonPressed(SDL_GAMEPAD_BUTTON_DPAD_UP);
   }

   return false;
}

bool PlayerControls::isDownButtonPressed() const
{
   if (!PlayerControlState::checkState())
   {
      return false;
   }

   if (_keys_pressed & KeyPressedDown)
   {
      return true;
   }

   if (GameControllerIntegration::getInstance().isControllerConnected())
   {
      return isControllerButtonPressed(SDL_GAMEPAD_BUTTON_DPAD_DOWN);
   }

   return false;
}

bool PlayerControls::isDroppingDown() const
{
   if (!PlayerControlState::checkState())
   {
      return false;
   }

   return isButtonAPressed() && isMovingDown(0.7f);
}

bool PlayerControls::isMovingLeft() const
{
   if (!PlayerControlState::checkState())
   {
      return false;
   }

   // check if button state is locked
   const auto it = readLockedState(KeyPressedLeft);
   if (it != _locked_keys.end())
   {
      return it->second.asBool();
   }

   // controller input
   if (GameControllerIntegration::getInstance().isControllerConnected())
   {
      const auto& axis_values = _joystick_info.getAxisValues();
      const auto axis_left_x = GameControllerIntegration::getInstance().getController()->getAxisIndex(SDL_GAMEPAD_AXIS_LEFTX);
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

bool PlayerControls::isMovingDown(float analog_threshold) const
{
   if (!PlayerControlState::checkState())
   {
      return false;
   }

   // controller input
   if (GameControllerIntegration::getInstance().isControllerConnected())
   {
      const auto& axis_values = _joystick_info.getAxisValues();
      const auto axis_left_y = GameControllerIntegration::getInstance().getController()->getAxisIndex(SDL_GAMEPAD_AXIS_LEFTY);
      auto y1 = axis_values[static_cast<size_t>(axis_left_y)] / 32767.0f;
      const auto hat_value = _joystick_info.getHatValues().at(0);
      const auto dpad_down_pressed = hat_value & SDL_HAT_DOWN;
      const auto dpad_up_pressed = hat_value & SDL_HAT_UP;

      if (dpad_down_pressed)
      {
         y1 = 1.0f;
      }
      else if (dpad_up_pressed)
      {
         y1 = -1.0f;
      }

      if (fabs(y1) > analog_threshold)
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

bool PlayerControls::isMovingUp(float analog_threshold) const
{
   if (!PlayerControlState::checkState())
   {
      return false;
   }

   // controller input
   if (GameControllerIntegration::getInstance().isControllerConnected())
   {
      const auto& axis_values = _joystick_info.getAxisValues();
      const auto axis_left_y = GameControllerIntegration::getInstance().getController()->getAxisIndex(SDL_GAMEPAD_AXIS_LEFTY);
      auto y1 = axis_values[static_cast<size_t>(axis_left_y)] / 32767.0f;
      const auto hat_value = _joystick_info.getHatValues().at(0);
      const auto dpad_down_pressed = hat_value & SDL_HAT_DOWN;
      const auto dpad_up_pressed = hat_value & SDL_HAT_UP;

      if (dpad_down_pressed)
      {
         y1 = 1.0f;
      }
      else if (dpad_up_pressed)
      {
         y1 = -1.0f;
      }

      if (fabs(y1) > analog_threshold)
      {
         if (y1 < 0.0f)
         {
            return true;
         }
      }
   }

   // keyboard input
   if (_keys_pressed & KeyPressedUp)
   {
      return true;
   }

   return false;
}

bool PlayerControls::isMovingRight() const
{
   if (!PlayerControlState::checkState())
   {
      return false;
   }

   // check if button state is locked
   const auto it = readLockedState(KeyPressedRight);
   if (it != _locked_keys.end())
   {
      return it->second.asBool();
   }

   // controller input
   if (GameControllerIntegration::getInstance().isControllerConnected())
   {
      const auto& axis_values = _joystick_info.getAxisValues();
      const auto axis_left_x = GameControllerIntegration::getInstance().getController()->getAxisIndex(SDL_GAMEPAD_AXIS_LEFTX);
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

bool PlayerControls::isMovingHorizontally() const
{
   return isMovingLeft() || isMovingRight();
}

int PlayerControls::getKeysPressed() const
{
   return _keys_pressed;
}

void PlayerControls::setKeysPressed(int32_t keysPressed)
{
   _keys_pressed = keysPressed;
}

const GameControllerInfo& PlayerControls::getJoystickInfo() const
{
   return _joystick_info;
}

void PlayerControls::setJoystickInfo(const GameControllerInfo& joystick_info)
{
   _joystick_info = joystick_info;
}

bool PlayerControls::wasMoving() const
{
   return _was_moving;
}

void PlayerControls::setWasMoving(bool was_moving)
{
   _was_moving = was_moving;
}

bool PlayerControls::wasMovingLeft() const
{
   return _was_moving_left;
}

void PlayerControls::setWasMovingLeft(bool was_moving_left)
{
   _was_moving_left = was_moving_left;
}

bool PlayerControls::wasMovingRight() const
{
   return _was_moving_right;
}

void PlayerControls::setWasMovingRight(bool was_moving_right)
{
   _was_moving_right = was_moving_right;
}

bool PlayerControls::changedToIdle() const
{
   return wasMoving() && !isMovingHorizontally();
}

bool PlayerControls::changedToMoving() const
{
   return !wasMoving() && isMovingHorizontally();
}

PlayerControls::Orientation PlayerControls::updateOrientation()
{
   if (!PlayerControlState::checkState())
   {
      return Orientation::Undefined;
   }

   Orientation orientation = Orientation::Undefined;

   // controller input
   if (GameControllerIntegration::getInstance().isControllerConnected())
   {
      const auto& axis_values = getJoystickInfo().getAxisValues();
      const auto axis_left_x = GameControllerIntegration::getInstance().getController()->getAxisIndex(SDL_GAMEPAD_AXIS_LEFTX);
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

bool PlayerControls::isBendDownActive() const
{
   if (!PlayerControlState::checkStateCpanOkay())
   {
      return false;
   }

   auto down_pressed = false;

   if (GameControllerIntegration::getInstance().isControllerConnected())
   {
      const auto& joystick_info = getJoystickInfo();
      const auto& axis_values = joystick_info.getAxisValues();

      const auto axis_lefy_y = GameControllerIntegration::getInstance().getController()->getAxisIndex(SDL_GAMEPAD_AXIS_LEFTY);
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

void PlayerControls::lockOrientation(std::chrono::milliseconds duration, Orientation orientation)
{
   if (orientation == Orientation::Undefined)
   {
      _locked_orientation = updateOrientation();
   }
   else
   {
      _locked_orientation = orientation;
   }

   const auto now = std::chrono::high_resolution_clock::now();
   _unlock_orientation_time_point = now + duration;
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
   const auto axis_left_x = GameControllerIntegration::getInstance().getController()->getAxisIndex(SDL_GAMEPAD_AXIS_LEFTX);
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

void PlayerControls::updateLockedKeys(const sf::Time& dt)
{
   const auto dt_chrono = std::chrono::milliseconds(dt.asMilliseconds());

   for (auto it = _locked_keys.begin(); it != _locked_keys.end();)
   {
      it->second._elapsed += dt_chrono;

      // remove if expired
      if (it->second._elapsed > it->second._locked_duration)
      {
         it = _locked_keys.erase(it);
      }
      else
      {
         ++it;
      }
   }
}

std::unordered_map<KeyPressed, PlayerControls::LockedKey>::const_iterator PlayerControls::readLockedState(KeyPressed key) const
{
   return _locked_keys.find(key);
}

void PlayerControls::lockState(KeyPressed key, LockedState state, const std::chrono::milliseconds& duration)
{
   if (key == KeyPressedLeft || key == KeyPressedRight)
   {
      _locked_keys[key] = {duration, state, {}};
   }
   else
   {
      Log::Error() << "unsupported key";
   }
}

void PlayerControls::lockAll(LockedState state, const std::chrono::milliseconds& duration)
{
   static const std::initializer_list<KeyPressed> keypress_types{
      KeyPressedUp,
      KeyPressedDown,
      KeyPressedLeft,
      KeyPressedRight,
      KeyPressedJump,
      KeyPressedAction,
      KeyPressedSlot1,
      KeyPressedSlot2,
      KeyPressedLook
   };

   for (auto keypress_type : keypress_types)
   {
      lockState(keypress_type, state, duration);
   }
}

bool PlayerControls::LockedKey::asBool() const
{
   if (_state == LockedState::Pressed)
   {
      return true;
   }

   return false;
}

float PlayerControls::readControllerNormalizedHorizontal() const
{
   if (!PlayerControlState::checkState())
   {
      return 0.0f;
   }

   // check if button state is locked
   const auto it_left = readLockedState(KeyPressedLeft);
   const auto it_right = readLockedState(KeyPressedRight);
   if (it_left != _locked_keys.end() && it_left->second._state == LockedState::Pressed)
   {
      return -1.0f;
   }
   if (it_right != _locked_keys.end() && it_right->second._state == LockedState::Pressed)
   {
      return 1.0f;
   }

   // analogue input normalized to -1..1
   const auto& axis_values = getJoystickInfo().getAxisValues();
   const auto axis_value = GameControllerIntegration::getInstance().getController()->getAxisIndex(SDL_GAMEPAD_AXIS_LEFTX);
   auto axis_value_normalized = axis_values[static_cast<size_t>(axis_value)] / 32767.0f;

   // digital input
   const auto hat_value = getJoystickInfo().getHatValues().at(0);
   const auto dpad_left_pressed = hat_value & SDL_HAT_LEFT;
   const auto dpad_right_pressed = hat_value & SDL_HAT_RIGHT;

   if (dpad_left_pressed)
   {
      axis_value_normalized = -1.0f;
   }
   else if (dpad_right_pressed)
   {
      axis_value_normalized = 1.0f;
   }

   return axis_value_normalized;
}
