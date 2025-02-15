#include "framework/joystick/gamecontroller.h"

#include "framework/tools/log.h"
#include "framework/tools/timer.h"

#include <algorithm>
#include <iostream>

GameController::~GameController()
{
   SDL_CloseGamepad(_gamepad);
}

/*!
  \param id joystick id
  \return joystick name1
*/
std::string GameController::getName(int32_t joystick_id) const
{
   return SDL_GetJoystickNameForID(joystick_id);
}

/*!
   \return axis count for joystick with given axis
*/
int32_t GameController::getAxisCount()
{
   return SDL_GetNumJoystickAxes(_joystick);
}

/*!
   \return ball count for joystick with given id
*/
int32_t GameController::getBallCount()
{
   return SDL_GetNumJoystickBalls(_joystick);
}

/*!
   \return hat count for joystick with given id
*/
int32_t GameController::getHatCount()
{
   return SDL_GetNumJoystickHats(_joystick);
}

/*!
   \param id of active joystick
*/
void GameController::activate(int32_t id)
{
   if (SDL_IsGamepad(id))
   {
      // store controller data
      _gamepad = SDL_OpenGamepad(id);
      _joystick = SDL_GetGamepadJoystick(_gamepad);

      // create dpad bindings
      bindDpadButtons();
   }
   else
   {
      _joystick = SDL_OpenJoystick(id);
   }
}

/*!
   \return id of active joystick
*/
int32_t GameController::getActiveJoystickId()
{
   return SDL_GetJoystickID(_joystick);
}

/*!
   \return number of joysticks
*/
int32_t GameController::getJoystickCount() const
{
   int32_t count = 0;
   SDL_GetJoysticks(&count);
   return count;
}

void GameController::callPressedCallbacks(const SDL_GamepadButton button)
{
   auto it = _button_pressed_callbacks.find(button);
   if (it != _button_pressed_callbacks.end())
   {
      for (const auto& callback : it->second)
      {
         callback();
      }
   }
}

void GameController::callReleasedCallbacks(const SDL_GamepadButton button)
{
   auto it = _button_released_callbacks.find(button);
   if (it != _button_released_callbacks.end())
   {
      for (const auto& callback : it->second)
      {
         callback();
      }
   }
}

/*!
  \param joystick info object
*/
void GameController::update()
{
   GameControllerInfo info;

   SDL_UpdateJoysticks();

   // read axis values
   for (auto axis = 0; axis < SDL_GetNumJoystickAxes(_joystick); axis++)
   {
      auto value = SDL_GetJoystickAxis(_joystick, axis);
      info.addAxisValue(value);
   }

   if (!_info.getAxisValues().empty())
   {
      for (auto& thresholds : _threshold_callbacks)
      {
         const auto axis = thresholds.first;

         for (auto& tc : thresholds.second)
         {
            const auto axis_index = getAxisIndex(axis);

            const auto value_previous = _info.getAxisValues().at(static_cast<size_t>(axis_index));
            const auto value_current = info.getAxisValues().at(static_cast<size_t>(axis_index));

            const auto value_current_normalized = value_current / 32767.0f;
            const auto value_previous_normalized = tc._value;

            const auto threshold = tc._threshold;

            // do not bother if value hasn't changed at all
            if (value_current != value_previous)
            {
               // threshold value must be initialized
               if (tc._initialized)
               {
                  // check if upper boundary was exceeded
                  if (tc._boundary == ThresholdCallback::Boundary::Upper)
                  {
                     // the previous value was outside the threshold, but the new one is -> fire callback
                     if (value_previous_normalized < threshold && value_current_normalized > threshold)
                     {
                        tc._callback();
                     }
                  }
                  else if (tc._boundary == ThresholdCallback::Boundary::Lower)
                  {
                     // the previous value was outside the threshold, but the new one is -> fire callback
                     if (value_previous_normalized > threshold && value_current_normalized < threshold)
                     {
                        tc._callback();
                     }
                  }
               }
            }

            // store current value
            tc._initialized = true;
            tc._value = value_current_normalized;
         }
      }
   }

   // read button values
   for (auto i = 0; i < SDL_GAMEPAD_BUTTON_COUNT; i++)
   {
      // do not place bombs on dpad pressed, but also do not screw up the
      // button id order
      if (isDpadButton(i))
      {
         info.addButtonState(false);
      }
      else
      {
         const auto pressed = SDL_GetGamepadButton(_gamepad, static_cast<SDL_GamepadButton>(i));
         info.addButtonState(pressed);
      }
   }

   // emulate hat by evaluating the dpad buttons. some drivers do not register
   // the controller's dpad as hat so they just show up as ordinary buttons.
   // we don't want that.
   auto hat_count = SDL_GetNumJoystickHats(_joystick);
   if (hat_count == 0)
   {
      auto hat = SDL_HAT_CENTERED;

      const auto up = static_cast<bool>(SDL_GetGamepadButton(_gamepad, SDL_GAMEPAD_BUTTON_DPAD_UP));
      const auto down = static_cast<bool>(SDL_GetGamepadButton(_gamepad, SDL_GAMEPAD_BUTTON_DPAD_DOWN));
      const auto left = static_cast<bool>(SDL_GetGamepadButton(_gamepad, SDL_GAMEPAD_BUTTON_DPAD_LEFT));
      const auto right = static_cast<bool>(SDL_GetGamepadButton(_gamepad, SDL_GAMEPAD_BUTTON_DPAD_RIGHT));

      if (left && up)
      {
         hat = SDL_HAT_LEFTUP;
      }
      else if (left && down)
      {
         hat = SDL_HAT_LEFTDOWN;
      }
      else if (right && up)
      {
         hat = SDL_HAT_RIGHTUP;
      }
      else if (right && down)
      {
         hat = SDL_HAT_RIGHTDOWN;
      }
      else if (up)
      {
         hat = SDL_HAT_UP;
      }
      else if (down)
      {
         hat = SDL_HAT_DOWN;
      }
      else if (left)
      {
         hat = SDL_HAT_LEFT;
      }
      else if (right)
      {
         hat = SDL_HAT_RIGHT;
      }

      info.addHatValue(hat);
   }

   // read hat values
   for (auto i = 0; i < hat_count; i++)
   {
      auto hat_value = SDL_GetJoystickHat(_joystick, i);
      info.addHatValue(hat_value);
   }

   if (!_info.getButtonValues().empty() && _info.getButtonValues().size() == info.getButtonValues().size())
   {
      for (auto button = 0u; button < SDL_GAMEPAD_BUTTON_COUNT; button++)
      {
         const auto previous = _info.getButtonValues().at(button);
         const auto current = info.getButtonValues().at(button);

         if (!previous && current)
         {
            callPressedCallbacks(static_cast<SDL_GamepadButton>(button));
         }

         if (previous && !current)
         {
            callReleasedCallbacks(static_cast<SDL_GamepadButton>(button));
         }
      }
   }

   _info = info;
}

void GameController::rumbleTest()
{
   rumble(1.0, 2000);
}

void GameController::rumble(float intensity, int32_t rumble_duration_ms)
{
   if (!_joystick)
   {
      return;
   }

   SDL_RumbleGamepad(_gamepad, static_cast<uint16_t>(0xffff * intensity), static_cast<uint16_t>(0xffff * intensity), rumble_duration_ms);
}

SDL_GamepadButton GameController::getButtonType(int32_t button_id) const
{
   int32_t count = 0;
   SDL_GamepadBinding** bindings = SDL_GetGamepadBindings(_gamepad, &count);
   SDL_GamepadButton button_type = SDL_GAMEPAD_BUTTON_INVALID;

   for (int i = 0; i < count; ++i)
   {
      auto* binding = bindings[i];
      if (binding->input_type == SDL_GAMEPAD_BINDTYPE_BUTTON && binding->input.button == button_id)
      {
         button_type = static_cast<SDL_GamepadButton>(binding->output.button);
         break;
      }
   }

   SDL_free(bindings);
   return button_type;
}

int32_t GameController::getAxisIndex(SDL_GamepadAxis axis) const
{
   int32_t count = 0;
   SDL_GamepadBinding** bindings = SDL_GetGamepadBindings(_gamepad, &count);

   for (auto i = 0; i < count; ++i)
   {
      auto* binding = bindings[i];
      if (binding->output_type == SDL_GAMEPAD_BINDTYPE_AXIS && binding->input_type == SDL_GAMEPAD_BINDTYPE_AXIS &&
          binding->output.axis.axis == axis)
      {
         const auto axis_index = binding->input.axis.axis;
         SDL_free(bindings);
         return axis_index;
      }
   }

   SDL_free(bindings);
   return -1;
}

const GameControllerInfo& GameController::getInfo() const
{
   return _info;
}

void GameController::addButtonPressedCallback(SDL_GamepadButton button, const ControllerCallback& callback)
{
   _button_pressed_callbacks[button].push_back(callback);
}

void GameController::removeButtonPressedCallback(SDL_GamepadButton button, const ControllerCallback& callback)
{
   auto& vec = _button_pressed_callbacks[button];
   vec.erase(
      std::remove_if(
         vec.begin(),
         vec.end(),
         [&](const ControllerCallback& c)
         {
            const auto match =
               c.target_type() == callback.target_type() && c.target<ControllerCallback>() == callback.target<ControllerCallback>();

            return match;
         }
      ),
      vec.end()
   );
}

void GameController::addButtonReleasedCallback(SDL_GamepadButton button, const ControllerCallback& callback)
{
   _button_released_callbacks[button].push_back(callback);
}

void GameController::addAxisThresholdExceedCallback(const ThresholdCallback& threshold)
{
   _threshold_callbacks[threshold._axis].push_back(threshold);
}

void GameController::bindDpadButtons()
{
   int32_t count = 0;
   SDL_GamepadBinding** bindings = SDL_GetGamepadBindings(_gamepad, &count);

   auto find_binding = [&](SDL_GamepadButton button) -> SDL_GamepadBinding
   {
      for (int i = 0; i < count; ++i)
      {
         auto* binding = bindings[i];

         if (binding->output_type == SDL_GAMEPAD_BINDTYPE_BUTTON && binding->input_type == SDL_GAMEPAD_BINDTYPE_AXIS &&
             binding->output.button == button)
         {
            return *binding;
         }
      }

      SDL_GamepadBinding empty_binding{};
      empty_binding.input_type = SDL_GAMEPAD_BINDTYPE_NONE;
      return empty_binding;
   };

   _dpad_bind_up = find_binding(SDL_GAMEPAD_BUTTON_DPAD_UP);
   _dpad_bind_down = find_binding(SDL_GAMEPAD_BUTTON_DPAD_DOWN);
   _dpad_bind_left = find_binding(SDL_GAMEPAD_BUTTON_DPAD_LEFT);
   _dpad_bind_right = find_binding(SDL_GAMEPAD_BUTTON_DPAD_RIGHT);

   auto reset_binding = [](SDL_GamepadBinding& bind)
   {
      if (bind.input_type == SDL_GAMEPAD_BINDTYPE_NONE)
      {
         if (bind.output_type == SDL_GAMEPAD_BINDTYPE_BUTTON)
         {
            bind.input.button = -1;
         }
         else if (bind.output_type == SDL_GAMEPAD_BINDTYPE_AXIS)
         {
            bind.input.axis.axis = -1;
            bind.input.axis.axis_min = -1;
            bind.input.axis.axis_max = -1;
         }
         else if (bind.output_type == SDL_GAMEPAD_BINDTYPE_HAT)
         {
            bind.input.hat.hat = -1;
            bind.input.hat.hat_mask = -1;
         }
      }
   };

   reset_binding(_dpad_bind_up);
   reset_binding(_dpad_bind_down);
   reset_binding(_dpad_bind_left);
   reset_binding(_dpad_bind_right);
}

/*!
   \return \c true if button is a dpad button
*/
bool GameController::isDpadButton(int32_t button) const
{
   return (
      (_dpad_bind_up.input_type == SDL_GAMEPAD_BINDTYPE_BUTTON && button == _dpad_bind_up.input.button) ||
      (_dpad_bind_down.input_type == SDL_GAMEPAD_BINDTYPE_BUTTON && button == _dpad_bind_down.input.button) ||
      (_dpad_bind_left.input_type == SDL_GAMEPAD_BINDTYPE_BUTTON && button == _dpad_bind_left.input.button) ||
      (_dpad_bind_right.input_type == SDL_GAMEPAD_BINDTYPE_BUTTON && button == _dpad_bind_right.input.button)
   );
}
