#include "camerapane.h"

#include "displaymode.h"
#include "gameconfiguration.h"
#include "gamecontrollerdata.h"
#include "gamecontrollerintegration.h"
#include "framework/joystick/gamecontroller.h"
#include "framework/math/sfmlmath.h"


CameraPane CameraPane::__instance;


//-----------------------------------------------------------------------------
CameraPane& CameraPane::getInstance()
{
   return __instance;
}


//-----------------------------------------------------------------------------
void CameraPane::update()
{
  constexpr auto speed = 3.0f;
  constexpr auto max_length = 100.0f;

  if (GameControllerData::getInstance().isControllerUsed())
  {
      auto axis_values = GameControllerData::getInstance().getJoystickInfo().getAxisValues();

      auto x_axis = GameControllerIntegration::getInstance(0)->getController()->getAxisIndex(SDL_CONTROLLER_AXIS_RIGHTX);
      auto y_axis = GameControllerIntegration::getInstance(0)->getController()->getAxisIndex(SDL_CONTROLLER_AXIS_RIGHTY);

      auto x = axis_values[static_cast<uint32_t>(x_axis)] / 32767.0f;
      auto y = axis_values[static_cast<uint32_t>(y_axis)] / 32767.0f;

      if (fabs(x) > 0.1f || fabs(y) > 0.1f)
      {
         auto w = GameConfiguration::getInstance()._view_width * 0.5f;
         auto h = GameConfiguration::getInstance()._view_height * 0.5f;

         _look_vector.x = x * w;
         _look_vector.y = y * h;
      }
      else
      {
         _look_vector.x = 0.0f;
         _look_vector.y = 0.0f;
      }
   }
   else if (_look_state & LookActive)
   {
      if (_look_state & LookUp)
      {
         _look_vector += sf::Vector2f(0.0f, -speed);
      }
      if (_look_state & LookDown)
      {
         _look_vector += sf::Vector2f(0.0f, speed);
      }
      if (_look_state & LookLeft)
      {
         _look_vector += sf::Vector2f(-speed, 0.0f);
      }
      if (_look_state & LookRight)
      {
         _look_vector += sf::Vector2f(speed, 0.0f);
      }

      if (!DisplayMode::getInstance().isSet(Display::DisplayMap))
      {
         auto len = SfmlMath::length(_look_vector);
         if (len > max_length)
         {
            _look_vector = SfmlMath::normalize(_look_vector);
            _look_vector *= max_length;
         }
      }
   }
   else
   {
      _look_vector *= 0.85f;
   }
}


//-----------------------------------------------------------------------------
void CameraPane::updateLookState(Look look, bool enable)
{
   if (enable)
   {
      _look_state |= look;
   }
   else
   {
      _look_state &= ~look;
   }
}




//-----------------------------------------------------------------------------
bool CameraPane::isLookActive() const
{
   return (_look_state & LookActive);
}


const sf::Vector2f& CameraPane::getLookVector() const
{
    return _look_vector;
}

