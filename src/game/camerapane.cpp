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
   const auto& tweaks = Tweaks::instance();

   auto limit_look_vector = [&](){
      if (!DisplayMode::getInstance().isSet(Display::Map))
      {
         const auto len = SfmlMath::length(_look_vector);
         if (len > tweaks._cpan_max_distance_px)
         {
            _look_vector = SfmlMath::normalize(_look_vector);
            _look_vector *= tweaks._cpan_max_distance_px;
         }
      }
   };

   if (_look_state & static_cast<int32_t>(Look::Active))
   {
      constexpr auto speed = 3.0f;

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

      limit_look_vector();
   }
   else if (GameControllerIntegration::getInstance().isControllerConnected())
   {
      const auto& axis_values = GameControllerData::getInstance().getJoystickInfo().getAxisValues();
      const auto x_axis = GameControllerIntegration::getInstance().getController()->getAxisIndex(SDL_CONTROLLER_AXIS_RIGHTX);
      const auto y_axis = GameControllerIntegration::getInstance().getController()->getAxisIndex(SDL_CONTROLLER_AXIS_RIGHTY);
      const auto x_normalized = axis_values[static_cast<uint32_t>(x_axis)] / 32767.0f;
      const auto y_normalized = axis_values[static_cast<uint32_t>(y_axis)] / 32767.0f;
      const auto tolerance_x = tweaks._cpan_tolerance_x;
      const auto tolerance_y = tweaks._cpan_tolerance_y;

      if (fabs(x_normalized) > tolerance_x || fabs(y_normalized) > tolerance_y)
      {
         const auto x_direction = std::signbit(x_normalized) ? -1.0f : 1.0f;
         const auto y_direction = std::signbit(y_normalized) ? -1.0f : 1.0f;
         const auto x_relative = x_direction * (fabs(x_normalized) - tolerance_x) / (1.0f - tolerance_x);
         const auto y_relative = y_direction * (fabs(y_normalized) - tolerance_y) / (1.0f - tolerance_y);
         _look_vector.x += x_relative * tweaks._cpan_look_speed_x;
         _look_vector.y += y_relative * tweaks._cpan_look_speed_y;

         limit_look_vector();
      }
      else
      {
         _look_vector *= tweaks._cpan_snap_back_factor;
      }
   }
   else
   {
      _look_vector *= tweaks._cpan_snap_back_factor;
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


//-----------------------------------------------------------------------------
const sf::Vector2f& CameraPane::getLookVector() const
{
    return _look_vector;
}

