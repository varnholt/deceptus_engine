// header
#include "gamecontroller.h"
#include "game/timer.h"


//-----------------------------------------------------------------------------
/*!
   \param parent parent widget
*/
GameController::GameController()
{
   SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC);

   // SDL_GameControllerAddMapping();
   auto res = SDL_GameControllerAddMappingsFromFile("data/joystick/gamecontrollerdb.txt");

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
   SDL_GameControllerClose(mController);
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
      count = SDL_JoystickNumAxes(mActiveJoystick);
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
      count = SDL_JoystickNumBalls(mActiveJoystick);
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
      count = SDL_JoystickNumHats(mActiveJoystick);
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
         mController = SDL_GameControllerOpen(id);
         mActiveJoystick = SDL_GameControllerGetJoystick(mController);

         // create dpad bindings
         bindDpadButtons();
      }
      else
      {
         mActiveJoystick = SDL_JoystickOpen(id);
      }
   }
}


//-----------------------------------------------------------------------------
/*!
   \return id of active joystick
*/
int GameController::getActiveJoystick()
{
   return SDL_JoystickInstanceID(mActiveJoystick);
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
   for (auto axis = 0; axis < SDL_JoystickNumAxes(mActiveJoystick); axis++)
   {
      auto value = SDL_JoystickGetAxis(mActiveJoystick, axis);
      info.addAxisValue(value);
   }

   if (!mInfo.getAxisValues().empty())
   {
      for (auto& thresholdInfo : mThresholdCallbacks)
      {
         const auto axis = thresholdInfo.first;
         const auto axisIndex = getAxisIndex(axis);

         const auto valuePrevious = mInfo.getAxisValues().at(static_cast<size_t>(axisIndex));
         const auto valueCurrent = info.getAxisValues().at(static_cast<size_t>(axisIndex));

         const auto valueCurrentNormalized = valueCurrent / 32767.0f;
         const auto valuePreviousNormalized = thresholdInfo.second.mValue;

         const auto threshold = thresholdInfo.second.mThreshold;

         // do not bother if value hasn't changed at all
         if (valueCurrent != valuePrevious)
         {
            // threshold value must be initialized
            if (valuePreviousNormalized > 0.0f)
            {
               // check if upper boundary was exceeded
               if (thresholdInfo.second.mBoundary == ThresholdCallback::Boundary::Upper)
               {
                  // the previous value was outside the threshold, but the new one is -> fire callback
                  if (
                        valuePreviousNormalized < threshold
                     && valueCurrentNormalized > threshold
                  )
                  {
                     thresholdInfo.second.mCallback();
                  }
               }
               else if (thresholdInfo.second.mBoundary == ThresholdCallback::Boundary::Lower)
               {
                  // the previous value was outside the threshold, but the new one is -> fire callback
                  if (
                        valuePreviousNormalized > threshold
                     && valueCurrentNormalized < threshold
                  )
                  {
                     thresholdInfo.second.mCallback();
                  }
               }
            }
         }

         // store current value
         thresholdInfo.second.mValue = valueCurrentNormalized;
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
         bool pressed = SDL_GameControllerGetButton(mController, static_cast<SDL_GameControllerButton>(i));
         info.addButtonState(pressed);
      }
   }

   // emulate hat by evaluating the dpad buttons. some drivers do not register
   // the controller's dpad as hat so they just show up as ordinary buttons.
   // we don't want that.
   auto hatCount = SDL_JoystickNumHats(mActiveJoystick);
   if (hatCount == 0)
   {
      int hat = SDL_HAT_CENTERED;

      bool up    = SDL_GameControllerGetButton(mController, SDL_CONTROLLER_BUTTON_DPAD_UP);
      bool down  = SDL_GameControllerGetButton(mController, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
      bool left  = SDL_GameControllerGetButton(mController, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
      bool right = SDL_GameControllerGetButton(mController, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);

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
      auto hatValue = SDL_JoystickGetHat(mActiveJoystick, i);
      info.addHatValue(hatValue);
   }

   if (
         !mInfo.getButtonValues().empty()
       && mInfo.getButtonValues().size() == info.getButtonValues().size()
   )
   {
      for (auto button = 0u; button < SDL_CONTROLLER_BUTTON_MAX; button++)
      {
         auto pre = mInfo.getButtonValues().at(button);
         auto cur = info.getButtonValues().at(button);

         if (!pre && cur)
         {
            auto it = mButtonPressedCallbacks.find(static_cast<SDL_GameControllerButton>(button));
            if (it != mButtonPressedCallbacks.end())
            {
               for (auto f : it->second)
               {
                  f();
               }
            }
         }

         if (pre && !cur)
         {
            auto it = mButtonReleasedCallbacks.find(static_cast<SDL_GameControllerButton>(button));
            if (it != mButtonReleasedCallbacks.end())
            {
               for (auto f : it->second)
               {
                  f();
               }
            }
         }
      }
   }

   mInfo = info;
}


//-----------------------------------------------------------------------------
void GameController::rumbleTest()
{
   rumble(1.0, 2000);
}


//-----------------------------------------------------------------------------
void GameController::rumble(float intensity, int ms)
{
   if (!mHaptic)
   {
      if (mActiveJoystick)
      {
         // open the device
         mHaptic = SDL_HapticOpenFromJoystick(mActiveJoystick);

         if (mHaptic)
         {
            // initialize simple rumble
            if (SDL_HapticRumbleInit(mHaptic) == 0)
            {
               if (SDL_HapticRumblePlay(mHaptic, intensity, ms) == 0)
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
   if (mHaptic)
      SDL_HapticClose(mHaptic);

   mHaptic = nullptr;
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
         mController,
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
      mController,
      button
   );

   return bind.value.button;
}


//-----------------------------------------------------------------------------
int32_t GameController::getAxisIndex(SDL_GameControllerAxis axis) const
{
   SDL_GameControllerButtonBind bind = SDL_GameControllerGetBindForAxis(
      mController,
      axis
   );

   return bind.value.axis;
}


//-----------------------------------------------------------------------------
const GameControllerInfo& GameController::getInfo() const
{
   return mInfo;
}


//-----------------------------------------------------------------------------
void GameController::addButtonPressedCallback(SDL_GameControllerButton button, std::function<void ()> callback)
{
   mButtonPressedCallbacks[button].push_back(callback);
}


//-----------------------------------------------------------------------------
void GameController::addButtonReleasedCallback(SDL_GameControllerButton button, std::function<void ()> callback)
{
   mButtonReleasedCallbacks[button].push_back(callback);
}


//-----------------------------------------------------------------------------
void GameController::addAxisThresholdExceedCallback(const ThresholdCallback& threshold)
{
   mThresholdCallbacks[threshold.mAxis] = threshold;
}


//-----------------------------------------------------------------------------
void GameController::bindDpadButtons()
{
   mDpadUpBind =
      SDL_GameControllerGetBindForButton(
         mController,
         SDL_CONTROLLER_BUTTON_DPAD_UP
      );

   mDpadDownBind =
      SDL_GameControllerGetBindForButton(
         mController,
         SDL_CONTROLLER_BUTTON_DPAD_DOWN
      );

   mDpadLeftBind =
      SDL_GameControllerGetBindForButton(
         mController,
         SDL_CONTROLLER_BUTTON_DPAD_LEFT
      );

   mDpadRightBind =
      SDL_GameControllerGetBindForButton(
         mController,
         SDL_CONTROLLER_BUTTON_DPAD_RIGHT
      );

   if (mDpadUpBind.bindType == SDL_CONTROLLER_BINDTYPE_NONE)
   {
      mDpadUpBind.value.axis = -1;
      mDpadUpBind.value.button = -1;
      mDpadUpBind.value.hat.hat = -1;
      mDpadUpBind.value.hat.hat_mask = -1;
   }

   if (mDpadDownBind.bindType == SDL_CONTROLLER_BINDTYPE_NONE)
   {
      mDpadDownBind.value.axis = -1;
      mDpadDownBind.value.button = -1;
      mDpadDownBind.value.hat.hat = -1;
      mDpadDownBind.value.hat.hat_mask = -1;
   }

   if (mDpadLeftBind.bindType == SDL_CONTROLLER_BINDTYPE_NONE)
   {
      mDpadLeftBind.value.axis = -1;
      mDpadLeftBind.value.button = -1;
      mDpadLeftBind.value.hat.hat = -1;
      mDpadLeftBind.value.hat.hat_mask = -1;
   }

   if (mDpadRightBind.bindType == SDL_CONTROLLER_BINDTYPE_NONE)
   {
      mDpadRightBind.value.axis = -1;
      mDpadRightBind.value.button = -1;
      mDpadRightBind.value.hat.hat = -1;
      mDpadRightBind.value.hat.hat_mask = -1;
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
          (button == mDpadUpBind.value.button    && mDpadUpBind.bindType    == SDL_CONTROLLER_BINDTYPE_BUTTON)
       || (button == mDpadDownBind.value.button  && mDpadDownBind.bindType  == SDL_CONTROLLER_BINDTYPE_BUTTON)
       || (button == mDpadLeftBind.value.button  && mDpadLeftBind.bindType  == SDL_CONTROLLER_BINDTYPE_BUTTON)
       || (button == mDpadRightBind.value.button && mDpadRightBind.bindType == SDL_CONTROLLER_BINDTYPE_BUTTON)
   );

   return dPadButton;
}


