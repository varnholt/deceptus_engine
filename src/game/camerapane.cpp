#include "camerapane.h"

#include "displaymode.h"
#include "gameconfiguration.h"
#include "gamecontrollerdata.h"
#include "gamecontrollerintegration.h"
#include "framework/joystick/gamecontroller.h"
#include "framework/math/sfmlmath.h"


CameraPane CameraPane::sInstance;


//-----------------------------------------------------------------------------
CameraPane& CameraPane::getInstance()
{
   return sInstance;
}


//-----------------------------------------------------------------------------
void CameraPane::update()
{
  auto speed = 3.0f;
  auto maxLength = 100.0f;

  if (GameControllerData::getInstance().isControllerUsed())
  {
      auto axisValues = GameControllerData::getInstance().getJoystickInfo().getAxisValues();

      auto xAxis = GameControllerIntegration::getInstance(0)->getController()->getAxisIndex(SDL_CONTROLLER_AXIS_RIGHTX);
      auto yAxis = GameControllerIntegration::getInstance(0)->getController()->getAxisIndex(SDL_CONTROLLER_AXIS_RIGHTY);

      auto x = axisValues[static_cast<uint32_t>(xAxis)] / 32767.0f;
      auto y = axisValues[static_cast<uint32_t>(yAxis)] / 32767.0f;

      if (fabs(x) > 0.1f || fabs(y) > 0.1f)
      {
         auto w = GameConfiguration::getInstance().mViewWidth * 0.5f;
         auto h = GameConfiguration::getInstance().mViewHeight * 0.5f;

         mLookVector.x = x * w;
         mLookVector.y = y * h;
      }
      else
      {
         mLookVector.x = 0.0f;
         mLookVector.y = 0.0f;
      }
   }
   else if (mLook & LookActive)
   {
      if (mLook & LookUp)
      {
         mLookVector += sf::Vector2f(0.0f, -speed);
      }
      if (mLook & LookDown)
      {
         mLookVector += sf::Vector2f(0.0f, speed);
      }
      if (mLook & LookLeft)
      {
         mLookVector += sf::Vector2f(-speed, 0.0f);
      }
      if (mLook & LookRight)
      {
         mLookVector += sf::Vector2f(speed, 0.0f);
      }

      if (!DisplayMode::getInstance().isSet(Display::DisplayMap))
      {
         auto len = SfmlMath::length(mLookVector);
         if (len > maxLength)
         {
            mLookVector = SfmlMath::normalize(mLookVector);
            mLookVector *= maxLength;
         }
      }
   }
   else
   {
      mLookVector *= 0.85f;
   }
}


//-----------------------------------------------------------------------------
void CameraPane::updateLookState(Look look, bool enable)
{
   if (enable)
   {
      mLook |= look;
   }
   else
   {
      mLook &= ~look;
   }
}




//-----------------------------------------------------------------------------
bool CameraPane::isLookActive() const
{
   return (mLook & LookActive);
}

const sf::Vector2f& CameraPane::getLookVector() const
{
    return mLookVector;
}

