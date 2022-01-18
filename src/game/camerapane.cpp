#include "camerapane.h"

#include "displaymode.h"
#include "gameconfiguration.h"
#include "gamecontrollerdata.h"
#include "gamecontrollerintegration.h"
#include "framework/joystick/gamecontroller.h"
#include "framework/math/sfmlmath.h"
#include "tweaks.h"


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

   if (_look_state & static_cast<int32_t>(Look::Active))
   {
      if (_look_state & static_cast<int32_t>(Look::Up))
      {
         _look_vector += sf::Vector2f(0.0f, -speed);
      }
      if (_look_state & static_cast<int32_t>(Look::Down))
      {
         _look_vector += sf::Vector2f(0.0f, speed);
      }
      if (_look_state & static_cast<int32_t>(Look::Left))
      {
         _look_vector += sf::Vector2f(-speed, 0.0f);
      }
      if (_look_state & static_cast<int32_t>(Look::Right))
      {
         _look_vector += sf::Vector2f(speed, 0.0f);
      }

      if (!DisplayMode::getInstance().isSet(Display::Map))
      {
         auto len = SfmlMath::length(_look_vector);
         if (len > _max_length)
         {
            _look_vector = SfmlMath::normalize(_look_vector);
            _look_vector *= _max_length;
         }
      }
   }
   else if (GameControllerIntegration::getInstance().isControllerConnected())
   {
      auto axis_values = GameControllerData::getInstance().getJoystickInfo().getAxisValues();

      auto x_axis = GameControllerIntegration::getInstance().getController()->getAxisIndex(SDL_CONTROLLER_AXIS_RIGHTX);
      auto y_axis = GameControllerIntegration::getInstance().getController()->getAxisIndex(SDL_CONTROLLER_AXIS_RIGHTY);

      auto x = axis_values[static_cast<uint32_t>(x_axis)] / 32767.0f;
      auto y = axis_values[static_cast<uint32_t>(y_axis)] / 32767.0f;
      const auto tx = Tweaks::instance()._cpan_tolerance_x;
      const auto ty = Tweaks::instance()._cpan_tolerance_y;

      if (fabs(x) > tx || fabs(y) > ty)
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
   else
   {
      _look_vector *= 0.85f;
   }
}


//-----------------------------------------------------------------------------
void CameraPane::processKeyPressedEvents(const sf::Event& event)
{
   switch (event.key.code)
   {
      case sf::Keyboard::LShift:
      {
         updateLookState(Look::Active, true);
         break;
      }
      case sf::Keyboard::Left:
      {
         updateLookState(Look::Left, true);
         break;
      }
      case sf::Keyboard::Right:
      {
         updateLookState(Look::Right, true);
         break;
      }
      case sf::Keyboard::Up:
      {
         updateLookState(Look::Up, true);
         break;
      }
      case sf::Keyboard::Down:
      {
         updateLookState(Look::Down, true);
         break;
      }
      default:
      {
         break;
      }
   }
}


//-----------------------------------------------------------------------------
void CameraPane::processKeyReleasedEvents(const sf::Event& event)
{
   switch (event.key.code)
   {
      case sf::Keyboard::LShift:
      {
         updateLookState(Look::Active, false);
         break;
      }
      case sf::Keyboard::Left:
      {
         updateLookState(Look::Left, false);
         break;
      }
      case sf::Keyboard::Right:
      {
         updateLookState(Look::Right, false);
         break;
      }
      case sf::Keyboard::Up:
      {
         updateLookState(Look::Up, false);
         break;
      }
      case sf::Keyboard::Down:
      {
         updateLookState(Look::Down, false);
         break;
      }
      default:
      {
         break;
      }
   }
}


//-----------------------------------------------------------------------------
void CameraPane::updateLookState(Look look, bool enable)
{
   if (enable)
   {
      _look_state |= static_cast<int32_t>(look);
   }
   else
   {
      _look_state &= ~static_cast<int32_t>(look);
   }
}




//-----------------------------------------------------------------------------
bool CameraPane::isLookActive() const
{
   return (_look_state & static_cast<int32_t>(Look::Active));
}


const sf::Vector2f& CameraPane::getLookVector() const
{
    return _look_vector;
}

