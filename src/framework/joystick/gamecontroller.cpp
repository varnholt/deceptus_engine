// header
#include "gamecontroller.h"
#include "framework/tools/timer.h"

#include <algorithm>
#include <iostream>


//-----------------------------------------------------------------------------
/*!
   \param parent parent widget
*/
GameController::GameController()
{
   SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC);

   // SDL_GameControllerAddMapping();
   [[maybe_unused]] auto res = SDL_GameControllerAddMappingsFromFile("data/joystick/gamecontrollerdb.txt");

   // if (res == -1)
   // {
   //    printf("error loading gamecontrollerdb\n");
   // }
   // else
   // {
   //    printf("%d game controller mappings loaded\n", res);
   // }
}


//-----------------------------------------------------------------------------
/*!
*/
GameController::~GameController()
{
   SDL_GameControllerClose(_controller);
   SDL_QuitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC);
}


//-----------------------------------------------------------------------------
/*!
  \param id joystick id
  \return joystick name1
*/
std::string GameController::getName(int id) const
{
   std::string name;

   if (
         id < getJoystickCount()
      && id >= 0
   )
   {
      name = SDL_JoystickNameForIndex(id);
   }

   return name;
}


//-----------------------------------------------------------------------------
/*!
   \return axis count for joystick with given axis
*/
int GameController::getAxisCount(int id)
{
   auto count = 0;

   if (
         id < getJoystickCount()
      && id >= 0
   )
   {
      count = SDL_JoystickNumAxes(_active_joystick);
   }

   return count;
}


//-----------------------------------------------------------------------------
/*!
   \return ball count for joystick with given id
*/
int GameController::getBallCount(int id)
{
   auto count = 0;

   if (
         id < getJoystickCount()
      && id >= 0
   )
   {
      count = SDL_JoystickNumBalls(_active_joystick);
   }

   return count;
}

//-----------------------------------------------------------------------------
/*!
   \return hat count for joystick with given id
*/
int GameController::getHatCount(int id)
{
   auto count = 0;

   if (
         id < getJoystickCount()
      && id >= 0
   )
   {
      count = SDL_JoystickNumHats(_active_joystick);
   }

   return count;
}


//-----------------------------------------------------------------------------
/*!
   \param id of active joystick
*/
void GameController::setActiveJoystick(int id)
{
   if (
         id < getJoystickCount()
      && id >= 0
   )
   {
      if (SDL_IsGameController(id))
      {
         // store controller data
         _controller = SDL_GameControllerOpen(id);
         _active_joystick = SDL_GameControllerGetJoystick(_controller);

         // create dpad bindings
         bindDpadButtons();
      }
      else
      {
         _active_joystick = SDL_JoystickOpen(id);
      }
   }
}


//-----------------------------------------------------------------------------
/*!
   \return id of active joystick
*/
int GameController::getActiveJoystick()
{
   return SDL_JoystickInstanceID(_active_joystick);
}


//-----------------------------------------------------------------------------
/*!
   \return number of joysticks
*/
int GameController::getJoystickCount() const
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
   for (auto axis = 0; axis < SDL_JoystickNumAxes(_active_joystick); axis++)
   {
      auto value = SDL_JoystickGetAxis(_active_joystick, axis);
      info.addAxisValue(value);
   }

   if (!_info.getAxisValues().empty())
   {
      for (auto& thresholds : _threshold_callbacks)
      {
         const auto axis = thresholds.first;

         for (auto& tc : thresholds.second)
         {
            const auto axisIndex = getAxisIndex(axis);

            const auto valuePrevious = _info.getAxisValues().at(static_cast<size_t>(axisIndex));
            const auto valueCurrent = info.getAxisValues().at(static_cast<size_t>(axisIndex));

            const auto valueCurrentNormalized = valueCurrent / 32767.0f;
            const auto valuePreviousNormalized = tc._value;

            const auto threshold = tc._threshold;

            // std::cout << valueCurrentNormalized << std::endl;

            // do not bother if value hasn't changed at all
            if (valueCurrent != valuePrevious)
            {
               // threshold value must be initialized
               if (tc._initialized)
               {
                  // check if upper boundary was exceeded
                  if (tc._boundary == ThresholdCallback::Boundary::Upper)
                  {
                     // the previous value was outside the threshold, but the new one is -> fire callback
                     if (
                           valuePreviousNormalized < threshold
                        && valueCurrentNormalized > threshold
                     )
                     {
                        tc._callback();
                     }
                  }
                  else if (tc._boundary == ThresholdCallback::Boundary::Lower)
                  {
                     // the previous value was outside the threshold, but the new one is -> fire callback
                     if (
                           valuePreviousNormalized > threshold
                        && valueCurrentNormalized < threshold
                     )
                     {
                        tc._callback();
                     }
                  }
               }
            }

            // store current value
            tc._initialized = true;
            tc._value = valueCurrentNormalized;
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
         bool pressed = SDL_GameControllerGetButton(_controller, static_cast<SDL_GameControllerButton>(i));
         info.addButtonState(pressed);
      }
   }

   // emulate hat by evaluating the dpad buttons. some drivers do not register
   // the controller's dpad as hat so they just show up as ordinary buttons.
   // we don't want that.
   auto hatCount = SDL_JoystickNumHats(_active_joystick);
   if (hatCount == 0)
   {
      int hat = SDL_HAT_CENTERED;

      const auto up    = SDL_GameControllerGetButton(_controller, SDL_CONTROLLER_BUTTON_DPAD_UP);
      const auto down  = SDL_GameControllerGetButton(_controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
      const auto left  = SDL_GameControllerGetButton(_controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
      const auto right = SDL_GameControllerGetButton(_controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);

      if (left && up)
         hat = SDL_HAT_LEFTUP;
      else if (left && down)
         hat = SDL_HAT_LEFTDOWN;
      else if (right && up)
         hat = SDL_HAT_RIGHTUP;
      else if (right && down)
         hat = SDL_HAT_RIGHTDOWN;
      else if (up)
         hat = SDL_HAT_UP;
      else if (down)
         hat = SDL_HAT_DOWN;
      else if (left)
         hat = SDL_HAT_LEFT;
      else if (right)
         hat = SDL_HAT_RIGHT;

      info.addHatValue(hat);
   }

   // read hat values
   for (auto i = 0; i < hatCount; i++)
   {
      auto hatValue = SDL_JoystickGetHat(_active_joystick, i);
      info.addHatValue(hatValue);
   }

   if (
         !_info.getButtonValues().empty()
       && _info.getButtonValues().size() == info.getButtonValues().size()
   )
   {
      for (auto button = 0u; button < SDL_CONTROLLER_BUTTON_MAX; button++)
      {
         auto pre = _info.getButtonValues().at(button);
         auto cur = info.getButtonValues().at(button);

         if (!pre && cur)
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

         if (pre && !cur)
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
void GameController::rumble(float intensity, int ms)
{
   if (!_haptic)
   {
      if (_active_joystick)
      {
         // open the device
         _haptic = SDL_HapticOpenFromJoystick(_active_joystick);

         if (_haptic)
         {
            // initialize simple rumble
            if (SDL_HapticRumbleInit(_haptic) == 0)
            {
               if (SDL_HapticRumblePlay(_haptic, intensity, ms) == 0)
               {
                  Timer::add(std::chrono::milliseconds(ms), [this](){cleanupRumble();}, Timer::Type::Singleshot);
               }
            }
         }
      }
   }
}


//-----------------------------------------------------------------------------
void GameController::cleanupRumble()
{
   if (_haptic)
      SDL_HapticClose(_haptic);

   _haptic = nullptr;
}


//-----------------------------------------------------------------------------
SDL_GameControllerButton GameController::getButtonType(int buttonId) const
{
   SDL_GameControllerButton buttonType = SDL_CONTROLLER_BUTTON_INVALID;
   SDL_GameControllerButton tmpType = SDL_CONTROLLER_BUTTON_INVALID;

   SDL_GameControllerButtonBind binding;

   for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; i++)
   {
      tmpType = static_cast<SDL_GameControllerButton>(i);

      binding = SDL_GameControllerGetBindForButton(
         _controller,
         tmpType
      );

      if (binding.value.button == buttonId)
      {
         buttonType = tmpType;
         break;
      }
   }

   return buttonType;
}


//-----------------------------------------------------------------------------
int32_t GameController::getButtonId(SDL_GameControllerButton button) const
{
   SDL_GameControllerButtonBind bind = SDL_GameControllerGetBindForButton(
      _controller,
      button
   );

   return bind.value.button;
}


//-----------------------------------------------------------------------------
int32_t GameController::getAxisIndex(SDL_GameControllerAxis axis) const
{
   SDL_GameControllerButtonBind bind = SDL_GameControllerGetBindForAxis(
      _controller,
      axis
   );

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
   // std::cout<< "registering: " << &callback << std::endl;
   _button_pressed_callbacks[button].push_back(callback);
}


//-----------------------------------------------------------------------------
void GameController::removeButtonPressedCallback(
   SDL_GameControllerButton button,
   const ControllerCallback& callback
)
{
   auto& vec = _button_pressed_callbacks[button];
   vec.erase(
      std::remove_if(
         vec.begin(),
         vec.end(),
         [&](const ControllerCallback& c) {
            const auto match =
                  c.target_type() == callback.target_type()
               && c.target<ControllerCallback>() == callback.target<ControllerCallback>();

            return match;
         }
      )
   );
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
   _dpad_bind_up =
      SDL_GameControllerGetBindForButton(
         _controller,
         SDL_CONTROLLER_BUTTON_DPAD_UP
      );

   _dpad_bind_down =
      SDL_GameControllerGetBindForButton(
         _controller,
         SDL_CONTROLLER_BUTTON_DPAD_DOWN
      );

   _dpad_bind_left =
      SDL_GameControllerGetBindForButton(
         _controller,
         SDL_CONTROLLER_BUTTON_DPAD_LEFT
      );

   _dpad_bind_right =
      SDL_GameControllerGetBindForButton(
         _controller,
         SDL_CONTROLLER_BUTTON_DPAD_RIGHT
      );

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
bool GameController::isDpadButton(int button) const
{
   bool dPadButton = false;

   dPadButton = (
          (button == _dpad_bind_up.value.button    && _dpad_bind_up.bindType    == SDL_CONTROLLER_BINDTYPE_BUTTON)
       || (button == _dpad_bind_down.value.button  && _dpad_bind_down.bindType  == SDL_CONTROLLER_BINDTYPE_BUTTON)
       || (button == _dpad_bind_left.value.button  && _dpad_bind_left.bindType  == SDL_CONTROLLER_BINDTYPE_BUTTON)
       || (button == _dpad_bind_right.value.button && _dpad_bind_right.bindType == SDL_CONTROLLER_BINDTYPE_BUTTON)
   );

   return dPadButton;
}


