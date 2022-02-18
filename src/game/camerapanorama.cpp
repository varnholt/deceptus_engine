#include "camerapanorama.h"

#include <iostream>

#include "cameraroomlock.h"
#include "camerasystem.h"
#include "displaymode.h"
#include "gameconfiguration.h"
#include "gamecontrollerdata.h"
#include "gamecontrollerintegration.h"
#include "framework/joystick/gamecontroller.h"
#include "framework/math/sfmlmath.h"
#include "player/player.h"
#include "tweaks.h"

namespace
{
constexpr auto speed = 3.0f;
}


CameraPanorama CameraPanorama::__instance;


//-----------------------------------------------------------------------------
CameraPanorama& CameraPanorama::getInstance()
{
   return __instance;
}


//-----------------------------------------------------------------------------
void CameraPanorama::update()
{
   const auto& tweaks = Tweaks::instance();

   auto limit_look_vector = [&](sf::Vector2f& look_vector){
      if (!DisplayMode::getInstance().isSet(Display::Map))
      {
         const auto len = SfmlMath::length(look_vector);
         if (len > tweaks._cpan_max_distance_px)
         {
            look_vector = SfmlMath::normalize(look_vector);
            look_vector *= tweaks._cpan_max_distance_px;
         }
      }
   };

   // this should work on actual screen boundaries instead of creating a screen rectangle around the player
   // 1) get screen rectangle
   // 2) have 4 points on each side to check if they are outside the room rectangle
   // 3) if those points are outside the room boundaries, do not allow further movement

   auto locked_up    = false;
   auto locked_down  = false;
   auto locked_left  = false;
   auto locked_right = false;
   CameraRoomLock::readLockedSides(locked_left, locked_right, locked_up, locked_down);

   if (_look_state & static_cast<int32_t>(Look::Active))
   {
      // only update the desired look vector when boundaries are not exceeded
      sf::Vector2f desired_look_vector = _look_vector;
      if (_look_state & static_cast<int32_t>(Look::Up) &&! (locked_up && desired_look_vector.y < 0.0f))
      {
         desired_look_vector += sf::Vector2f(0.0f, -speed);
      }
      else if (_look_state & static_cast<int32_t>(Look::Down) &&! (locked_down && desired_look_vector.y > 0.0f))
      {
         desired_look_vector += sf::Vector2f(0.0f, speed);
      }

      if (_look_state & static_cast<int32_t>(Look::Left) &&! (locked_left && desired_look_vector.x < 0.0f))
      {
         desired_look_vector += sf::Vector2f(-speed, 0.0f);
      }
      else if (_look_state & static_cast<int32_t>(Look::Right) &&! (locked_right && desired_look_vector.x > 0.0f))
      {
         desired_look_vector += sf::Vector2f(speed, 0.0f);
      }

      limit_look_vector(desired_look_vector);
      updateLookVector(desired_look_vector);
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

         sf::Vector2f desired_look_vector = _look_vector;

         // only update the desired look vector when boundaries are not exceeded
         if (x_relative < 0.0f &&! (locked_left && desired_look_vector.x < 0.0f))
         {
            desired_look_vector.x += x_relative * tweaks._cpan_look_speed_x;
         }
         else if (x_relative > 0.0f &&! (locked_right && desired_look_vector.x > 0.0f))
         {
            desired_look_vector.x += x_relative * tweaks._cpan_look_speed_x;
         }

         if (y_relative < 0.0f &&! (locked_up && desired_look_vector.y < 0.0f))
         {
            desired_look_vector.y += y_relative * tweaks._cpan_look_speed_y;
         }
         else if (y_relative > 0.0f &&! (locked_down && desired_look_vector.y > 0.0f))
         {
            desired_look_vector.y += y_relative * tweaks._cpan_look_speed_y;
         }

         limit_look_vector(desired_look_vector);
         updateLookVector(desired_look_vector);
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
void CameraPanorama::processKeyPressedEvents(const sf::Event& event)
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
void CameraPanorama::processKeyReleasedEvents(const sf::Event& event)
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
void CameraPanorama::updateLookState(Look look, bool enable)
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
void CameraPanorama::updateLookVector(const sf::Vector2f& desired)
{
   _look_vector = desired;
}


//-----------------------------------------------------------------------------
bool CameraPanorama::isLookActive() const
{
   return (_look_state & static_cast<int32_t>(Look::Active));
}


//-----------------------------------------------------------------------------
const sf::Vector2f& CameraPanorama::getLookVector() const
{
    return _look_vector;
}


// so far not resolved issue, could be considered if needed:
//
// the player is allowed to move 'freely' within the camera system's focus zone (f0..f1).
// that's usually a horizontal range within the center of the screen. that range can be
// shifted around a bit depending on the player movement (if enabled in the camera system).
// however, when the player is more on the left or right side of the focus zone, then the
// range of the camera panorama should be adjusted.
//
//
// default use case A
// +-------------+-------------+
// |       :     |     :       |
// |       :     |     :       |
// |       :     |     :       |
// |       :     |     :       |
// +-------:-----p-----:-------+
// |       :     |     :       |
// |       :     |     :       |
// +-------------+-------------+
//         f0          f1
//
// outside of center use case B                        outside of center use case C
// +-----------------+---------+                       +---------+-----------------+
// |       :         | :       |                       |       : |         :       |
// |       :         | :       |                       |       : |         :       |
// |       :         | :       |                       |       : |         :       |
// |       :         | :       |                       |       : |         :       |
// +-------:-----|---p-:-------+                       +-------:-p---|-----:-------+
// |       :         | :       |                       |       : |         :       |
// |       :         | :       |                       |       : |         :       |
// +-----------------+---------+                       +---------+-----------------+
//         f0          f1                                      f0          f1
//
//                    player is on the right side              player is on the left side
//                    of the focus zone                        of the focus zone
//                    camera system dx = 90px                  camera system dx = -90
//
//
// in use case B, already a large portion of the left-facing side of the screen is visible (90px).
// by using the camera panorama these 90px should be subtracted from the cpan range when the user
// wants to navigate the cpan to the left.
//
// player is standing in the center of the focus zone (A)
//    dx = 0
//    -> cpan should go from -100; 100
//
// player standing on the right of the focus zone (B)
//    dx = 90
//    -> cpan should go from -10 .. 190
//
// player standing on the left of the focus zone (C)
//    dx = -90
//    -> cpan should go from -10
//
//    -> should go from -100 to 100
//
// std::cout << CameraSystem::getInstance().getDx() << " " << CameraSystem::getInstance().getDy() << std::endl;
