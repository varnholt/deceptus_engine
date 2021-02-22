#include "playercontrols.h"

#include "constants.h"
#include "gamecontrollerintegration.h"
#include "framework/joystick/gamecontroller.h"


//----------------------------------------------------------------------------------------------------------------------
void PlayerControls::addKeypressedCallback(const KeypressedCallback& callback)
{
   mKeypressedCallbacks.push_back(callback);
}


//----------------------------------------------------------------------------------------------------------------------
bool PlayerControls::hasFlag(int32_t flag) const
{
   return mKeysPressed & flag;
}


//----------------------------------------------------------------------------------------------------------------------
void PlayerControls::forceSync()
{
   if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space))
   {
      mKeysPressed |= KeyPressedJump;
   }

   if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift))
   {
      mKeysPressed |= KeyPressedLook;
   }

   if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
   {
      mKeysPressed |= KeyPressedUp;
   }

   if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
   {
      mKeysPressed |= KeyPressedDown;
   }

   if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
   {
      mKeysPressed |= KeyPressedLeft;
   }

   if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
   {
      mKeysPressed |= KeyPressedRight;
   }

   if (sf::Keyboard::isKeyPressed(sf::Keyboard::LAlt))
   {
      mKeysPressed |= KeyPressedRun;
   }

   if (sf::Keyboard::isKeyPressed(sf::Keyboard::LControl))
   {
      mKeysPressed |= KeyPressedFire;
   }
}


//----------------------------------------------------------------------------------------------------------------------
void PlayerControls::keyboardKeyPressed(sf::Keyboard::Key key)
{
   if (GameControllerIntegration::getCount() > 0)
   {
      return;
   }

   if (key == sf::Keyboard::Space)
   {
      mKeysPressed |= KeyPressedJump;
   }

   if (key == sf::Keyboard::LShift)
   {
      mKeysPressed |= KeyPressedLook;
   }

   if (key == sf::Keyboard::Up)
   {
      mKeysPressed |= KeyPressedUp;
   }

   if (key == sf::Keyboard::Down)
   {
      mKeysPressed |= KeyPressedDown;
   }

   if (key == sf::Keyboard::Left)
   {
      mKeysPressed |= KeyPressedLeft;
   }

   if (key == sf::Keyboard::Right)
   {
      mKeysPressed |= KeyPressedRight;
   }

   if (key == sf::Keyboard::LAlt)
   {
      mKeysPressed |= KeyPressedRun;
   }

   if (key == sf::Keyboard::LControl)
   {
      mKeysPressed |= KeyPressedFire;
   }

   for (auto& callback : mKeypressedCallbacks)
   {
      callback(key);
   }
}


//----------------------------------------------------------------------------------------------------------------------
void PlayerControls::keyboardKeyReleased(sf::Keyboard::Key key)
{
   if (GameControllerIntegration::getCount() > 0)
   {
      return;
   }

   if (key == sf::Keyboard::LShift)
   {
      mKeysPressed &= ~KeyPressedLook;
   }

   if (key == sf::Keyboard::Up)
   {
      mKeysPressed &= ~KeyPressedUp;
   }

   if (key == sf::Keyboard::Down)
   {
      mKeysPressed &= ~KeyPressedDown;
   }

   if (key == sf::Keyboard::Left)
   {
      mKeysPressed &= ~KeyPressedLeft;
   }

   if (key == sf::Keyboard::Right)
   {
      mKeysPressed &= ~KeyPressedRight;
   }

   if (key == sf::Keyboard::Space)
   {
      mKeysPressed &= ~KeyPressedJump;
   }

   if (key == sf::Keyboard::LAlt)
   {
      mKeysPressed &= ~KeyPressedRun;
   }

   if (key == sf::Keyboard::LControl)
   {
      mKeysPressed &= ~KeyPressedFire;
   }
}


//----------------------------------------------------------------------------------------------------------------------
bool PlayerControls::isLookingAround() const
{
  if (mKeysPressed & KeyPressedLook)
  {
    return true;
  }

  if (isControllerUsed())
  {
    return isControllerButtonPressed(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
  }

  return false;
}


//----------------------------------------------------------------------------------------------------------------------
bool PlayerControls::isControllerUsed() const
{
  return !mJoystickInfo.getAxisValues().empty();
}


//----------------------------------------------------------------------------------------------------------------------
bool PlayerControls::isControllerButtonPressed(int buttonEnum) const
{
  auto pressed = false;

  auto gji = GameControllerIntegration::getInstance(0);
  if (gji != nullptr)
  {
     auto buttonId = gji->getController()->getButtonId(static_cast<SDL_GameControllerButton>(buttonEnum));
     pressed = (mJoystickInfo.getButtonValues()[static_cast<size_t>(buttonId)]);
  }

  return pressed;
}


//----------------------------------------------------------------------------------------------------------------------
bool PlayerControls::isFireButtonPressed() const
{
  if (mKeysPressed & KeyPressedFire)
  {
    return true;
  }

  if (isControllerUsed())
  {
    return isControllerButtonPressed(SDL_CONTROLLER_BUTTON_X);
  }

  return false;
}


//----------------------------------------------------------------------------------------------------------------------
bool PlayerControls::isJumpButtonPressed() const
{
  if (mKeysPressed & KeyPressedJump)
  {
    return true;
  }

  if (isControllerUsed())
  {
     return isControllerButtonPressed(SDL_CONTROLLER_BUTTON_A);
  }

  return false;
}



//----------------------------------------------------------------------------------------------------------------------
bool PlayerControls::isMovingLeft() const
{
  if (isControllerUsed())
  {
     const auto& axisValues = mJoystickInfo.getAxisValues();
     int axisLeftX = GameControllerIntegration::getInstance(0)->getController()->getAxisIndex(SDL_CONTROLLER_AXIS_LEFTX);
     auto xl = axisValues[static_cast<size_t>(axisLeftX)] / 32767.0f;
     auto hatValue = mJoystickInfo.getHatValues().at(0);
     auto dpadLeftPressed = hatValue & SDL_HAT_LEFT;
     auto dpadRightPressed = hatValue & SDL_HAT_RIGHT;

     if (dpadLeftPressed)
     {
        xl = -1.0f;
     }
     else if (dpadRightPressed)
     {
        xl = 1.0f;
     }

     if (fabs(xl) >  0.3f)
     {
        if (xl < 0.0f)
        {
           return true;
        }
     }
  }
  else
  {
     if (mKeysPressed & KeyPressedLeft)
     {
        return true;
     }
  }

  return false;
}


//----------------------------------------------------------------------------------------------------------------------
bool PlayerControls::isMovingRight() const
{
  if (isControllerUsed())
  {
     const auto& axisValues = mJoystickInfo.getAxisValues();
     int axisLeftX = GameControllerIntegration::getInstance(0)->getController()->getAxisIndex(SDL_CONTROLLER_AXIS_LEFTX);
     auto xl = axisValues[static_cast<size_t>(axisLeftX)] / 32767.0f;
     auto hatValue = mJoystickInfo.getHatValues().at(0);
     auto dpadLeftPressed = hatValue & SDL_HAT_LEFT;
     auto dpadRightPressed = hatValue & SDL_HAT_RIGHT;

     if (dpadLeftPressed)
     {
        xl = -1.0f;
     }
     else if (dpadRightPressed)
     {
        xl = 1.0f;
     }

     if (fabs(xl)> 0.3f)
     {
        if (xl > 0.0f)
        {
           return true;
        }
     }
  }
  else
  {
     if (mKeysPressed & KeyPressedRight)
     {
        return true;
     }
  }

  return false;
}


//----------------------------------------------------------------------------------------------------------------------
bool PlayerControls::isMoving() const
{
   return isMovingLeft() || isMovingRight();
}


//----------------------------------------------------------------------------------------------------------------------
int PlayerControls::getKeysPressed() const
{
   return mKeysPressed;
}


//----------------------------------------------------------------------------------------------------------------------
void PlayerControls::setKeysPressed(int keysPressed)
{
   mKeysPressed = keysPressed;
}


//----------------------------------------------------------------------------------------------------------------------
const GameControllerInfo& PlayerControls::getJoystickInfo() const
{
   return mJoystickInfo;
}


//----------------------------------------------------------------------------------------------------------------------
void PlayerControls::setJoystickInfo(const GameControllerInfo &joystickInfo)
{
   mJoystickInfo = joystickInfo;
}
