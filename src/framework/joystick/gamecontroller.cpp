// header
#include "gamecontroller.h"
#include "framework/tools/log.h"
#include "framework/tools/timer.h"

#include <algorithm>
#include <iostream>

//-----------------------------------------------------------------------------
GameController::~GameController()
{
   SDL_GameControllerClose(_controller);
}

//-----------------------------------------------------------------------------
bool GameController::validId(int32_t id) const
{
   return (id < getJoystickCount()) && (id >= 0);
}

//-----------------------------------------------------------------------------
/*!
  \param id joystick id
  \return joystick name1
*/
std::string GameController::getName(int32_t id) const
{
   std::string name;

   if (validId(id))
   {
      name = SDL_JoystickNameForIndex(id);
   }

   return name;
}

//-----------------------------------------------------------------------------
/*!
   \return axis count for joystick with given axis
*/
int32_t GameController::getAxisCount()
{
   return SDL_JoystickNumAxes(_joystick);
}

//-----------------------------------------------------------------------------
/*!
   \return ball count for joystick with given id
*/
int32_t GameController::getBallCount()
{
   return SDL_JoystickNumBalls(_joystick);
}

//-----------------------------------------------------------------------------
/*!
   \return hat count for joystick with given id
*/
int32_t GameController::getHatCount()
{
   return SDL_JoystickNumHats(_joystick);
}

//-----------------------------------------------------------------------------
/*!
   \param id of active joystick
*/
void GameController::activate(int32_t id)
{
   if (!validId(id))
   {
      return;
   }

   if (SDL_IsGameController(id))
   {
      // store controller data
      _controller = SDL_GameControllerOpen(id);
      _joystick = SDL_GameControllerGetJoystick(_controller);

      // create dpad bindings
      bindDpadButtons();
   }
   else
   {
      _joystick = SDL_JoystickOpen(id);
   }
}

//-----------------------------------------------------------------------------
/*!
   \return id of active joystick
*/
int32_t GameController::getActiveJoystickId()
{
   return SDL_JoystickInstanceID(_joystick);
}

//-----------------------------------------------------------------------------
/*!
   \return number of joysticks
*/
int32_t GameController::getJoystickCount() const
{
   return SDL_NumJoysticks();
}

//-----------------------------------------------------------------------------
/*!
  \param joystick info object
*/
void GameController::update()
{
   GameControllerInfo info;

   SDL_JoystickUpdate();

   // read axis values
   for (auto axis = 0; axis < SDL_JoystickNumAxes(_joystick); axis++)
   {
      auto value = SDL_JoystickGetAxis(_joystick, axis);
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
   for (auto i = 0; i < SDL_CONTROLLER_BUTTON_MAX; i++)
   {
      // do not place bombs on dpad pressed, but also do not screw up the
      // button id order
      if (isDpadButton(i))
      {
         info.addButtonState(false);
      }
      else
      {
         const auto pressed = SDL_GameControllerGetButton(_controller, static_cast<SDL_GameControllerButton>(i));
         info.addButtonState(pressed);
      }
   }

   // emulate hat by evaluating the dpad buttons. some drivers do not register
   // the controller's dpad as hat so they just show up as ordinary buttons.
   // we don't want that.
   auto hat_count = SDL_JoystickNumHats(_joystick);
   if (hat_count == 0)
   {
      auto hat = SDL_HAT_CENTERED;

      const auto up = SDL_GameControllerGetButton(_controller, SDL_CONTROLLER_BUTTON_DPAD_UP);
      const auto down = SDL_GameControllerGetButton(_controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
      const auto left = SDL_GameControllerGetButton(_controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
      const auto right = SDL_GameControllerGetButton(_controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);

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
      auto hat_value = SDL_JoystickGetHat(_joystick, i);
      info.addHatValue(hat_value);
   }

   if (!_info.getButtonValues().empty() && _info.getButtonValues().size() == info.getButtonValues().size())
   {
      for (auto button = 0u; button < SDL_CONTROLLER_BUTTON_MAX; button++)
      {
         const auto previous = _info.getButtonValues().at(button);
         const auto current = info.getButtonValues().at(button);

         if (!previous && current)
         {
            auto it = _button_pressed_callbacks.find(static_cast<SDL_GameControllerButton>(button));
            if (it != _button_pressed_callbacks.end())
            {
               for (auto& f : it->second)
               {
                  f();
               }
            }
         }

         if (previous && !current)
         {
            auto it = _button_released_callbacks.find(static_cast<SDL_GameControllerButton>(button));
            if (it != _button_released_callbacks.end())
            {
               for (auto& f : it->second)
               {
                  f();
               }
            }
         }
      }
   }

   _info = info;
}

//-----------------------------------------------------------------------------
void GameController::rumbleTest()
{
   rumble(1.0, 2000);
}

//-----------------------------------------------------------------------------
void GameController::rumble(float intensity, int32_t ms)
{
   if (!_joystick)
   {
      return;
   }

   SDL_GameControllerRumble(_controller, 0xffff * intensity, 0xffff * intensity, ms);
}

//-----------------------------------------------------------------------------
SDL_GameControllerButton GameController::getButtonType(int32_t button_id) const
{
   SDL_GameControllerButton button_type = SDL_CONTROLLER_BUTTON_INVALID;
   SDL_GameControllerButton tmp_type = SDL_CONTROLLER_BUTTON_INVALID;

   SDL_GameControllerButtonBind binding;

   for (auto i = 0; i < SDL_CONTROLLER_BUTTON_MAX; i++)
   {
      tmp_type = static_cast<SDL_GameControllerButton>(i);

      binding = SDL_GameControllerGetBindForButton(_controller, tmp_type);

      if (binding.value.button == button_id)
      {
         button_type = tmp_type;
         break;
      }
   }

   return button_type;
}

//-----------------------------------------------------------------------------
int32_t GameController::getButtonId(SDL_GameControllerButton button) const
{
   SDL_GameControllerButtonBind bind = SDL_GameControllerGetBindForButton(_controller, button);
   return bind.value.button;
}

//-----------------------------------------------------------------------------
int32_t GameController::getAxisIndex(SDL_GameControllerAxis axis) const
{
   SDL_GameControllerButtonBind bind = SDL_GameControllerGetBindForAxis(_controller, axis);
   return bind.value.axis;
}

//-----------------------------------------------------------------------------
const GameControllerInfo& GameController::getInfo() const
{
   return _info;
}

//-----------------------------------------------------------------------------
void GameController::addButtonPressedCallback(SDL_GameControllerButton button, const ControllerCallback& callback)
{
   _button_pressed_callbacks[button].push_back(callback);
}

//-----------------------------------------------------------------------------
void GameController::removeButtonPressedCallback(SDL_GameControllerButton button, const ControllerCallback& callback)
{
   auto& vec = _button_pressed_callbacks[button];
   vec.erase(std::remove_if(
      vec.begin(),
      vec.end(),
      [&](const ControllerCallback& c)
      {
         const auto match =
            c.target_type() == callback.target_type() && c.target<ControllerCallback>() == callback.target<ControllerCallback>();

         return match;
      }
   ));
}

//-----------------------------------------------------------------------------
void GameController::addButtonReleasedCallback(SDL_GameControllerButton button, const ControllerCallback& callback)
{
   _button_released_callbacks[button].push_back(callback);
}

//-----------------------------------------------------------------------------
void GameController::addAxisThresholdExceedCallback(const ThresholdCallback& threshold)
{
   _threshold_callbacks[threshold._axis].push_back(threshold);
}

//-----------------------------------------------------------------------------
void GameController::bindDpadButtons()
{
   _dpad_bind_up = SDL_GameControllerGetBindForButton(_controller, SDL_CONTROLLER_BUTTON_DPAD_UP);
   _dpad_bind_down = SDL_GameControllerGetBindForButton(_controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
   _dpad_bind_left = SDL_GameControllerGetBindForButton(_controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
   _dpad_bind_right = SDL_GameControllerGetBindForButton(_controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);

   if (_dpad_bind_up.bindType == SDL_CONTROLLER_BINDTYPE_NONE)
   {
      _dpad_bind_up.value.axis = -1;
      _dpad_bind_up.value.button = -1;
      _dpad_bind_up.value.hat.hat = -1;
      _dpad_bind_up.value.hat.hat_mask = -1;
   }

   if (_dpad_bind_down.bindType == SDL_CONTROLLER_BINDTYPE_NONE)
   {
      _dpad_bind_down.value.axis = -1;
      _dpad_bind_down.value.button = -1;
      _dpad_bind_down.value.hat.hat = -1;
      _dpad_bind_down.value.hat.hat_mask = -1;
   }

   if (_dpad_bind_left.bindType == SDL_CONTROLLER_BINDTYPE_NONE)
   {
      _dpad_bind_left.value.axis = -1;
      _dpad_bind_left.value.button = -1;
      _dpad_bind_left.value.hat.hat = -1;
      _dpad_bind_left.value.hat.hat_mask = -1;
   }

   if (_dpad_bind_right.bindType == SDL_CONTROLLER_BINDTYPE_NONE)
   {
      _dpad_bind_right.value.axis = -1;
      _dpad_bind_right.value.button = -1;
      _dpad_bind_right.value.hat.hat = -1;
      _dpad_bind_right.value.hat.hat_mask = -1;
   }
}

//-----------------------------------------------------------------------------
/*!
   \return \c true if button is a dpad button
*/
bool GameController::isDpadButton(int32_t button) const
{
   bool dpad_button =
      ((button == _dpad_bind_up.value.button && _dpad_bind_up.bindType == SDL_CONTROLLER_BINDTYPE_BUTTON) ||
       (button == _dpad_bind_down.value.button && _dpad_bind_down.bindType == SDL_CONTROLLER_BINDTYPE_BUTTON) ||
       (button == _dpad_bind_left.value.button && _dpad_bind_left.bindType == SDL_CONTROLLER_BINDTYPE_BUTTON) ||
       (button == _dpad_bind_right.value.button && _dpad_bind_right.bindType == SDL_CONTROLLER_BINDTYPE_BUTTON));

   return dpad_button;
}
