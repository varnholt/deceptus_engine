#include "camerasystem.h"

#include "cameraroomlock.h"
#include "framework/easings/easings.h"
#include "gameconfiguration.h"
#include "player/player.h"

#include <iostream>


/*

   Camera system

      Camera X position

         1) dist = distance between old camera x position and the current x player position
         2) dist /= 32
         3) dist = min(dist, max_camera_speed)
         4) have a focus zone which is a rectangle around the player
            if the player walks out of the focus zone, apply the offset
         5) that rectangle is moved further to the left if the player looks to the right and vice versa.
         6) if the player accelerates in a direction that rectangle is moved even further into the opposite direction.


      Camera Y position

         1) dist = distance between old camera y position and the current player y position
         2) dist /= 16
         3) the camera is only moved when the player touches the ground
         4) have two 'panic lines' at the top and bottom of the screen; if the player falls through those lines,
            move the camera


              f0          f1
               |           |
      ---------+-----------+--- p0
               |           |
               |    O      |
               |   `|`     |
               |    |\     |
               |    ###    |
               |    ###    |
      ---------+-----------+--- p1
      ###   ###|###########|###
      ###   ###|###########|###

*/


void CameraSystem::update(const sf::Time& dt, float view_width, float view_height)
{
   _view_width = view_width;
   _view_height = view_height;

   updateX(dt);
   updateY(dt);
}


void CameraSystem::updateX(const sf::Time& dt)
{
   auto& camera_config = CameraSystemConfiguration::getInstance();

   auto player = Player::getCurrent();
   auto player_x = player->getPixelPositionf().x;
   auto player_y = player->getPixelPositionf().y;
   const auto room_corrected = CameraRoomLock::correctedCamera(player_x, player_y, _focus_offset);
   const auto dx = (player_x - _x) * dt.asSeconds() * camera_config.getCameraVelocityFactorX();
   const auto f_center = _view_width / 2.0f;
   const auto f_range  = _view_width / camera_config.getFocusZoneDivider();

   _focus_zone_x0 = f_center - f_range;
   _focus_zone_x1 = f_center + f_range;

   // shift focus zone based on player orientation
   auto target_offset = 0.0f;
   if (camera_config.isFollowingPlayerOrientation())
   {
      target_offset = player->isPointingLeft()
         ? ( f_range * camera_config.getTargetShiftFactor())
         : (-f_range * camera_config.getTargetShiftFactor());
   }

   const auto focus_delta = (target_offset - _focus_offset) * dt.asSeconds() * camera_config.getCameraVelocityFactorX();
   if (fabs(_focus_offset) < fabs(f_range * camera_config.getTargetShiftFactor()))
   {
      _focus_offset += focus_delta;
   }

   _focus_zone_x0 += _focus_offset;
   _focus_zone_x1 += _focus_offset;
   _focus_zone_center = ((_focus_zone_x0 + _focus_zone_x1) / 2.0f);

   // test if out of focus zone boundaries
   const auto test = player_x - _focus_zone_center;
   const auto f0 = _x - _focus_zone_x1;
   const auto f1 = _x - _focus_zone_x0;

   if (test < f0 || test > f1)
   {
      _focus_x_triggered = true;
   }
   else if (
         (test > _x - _focus_zone_center - camera_config.getBackInBoundsToleranceX())
      && (test < _x - _focus_zone_center + camera_config.getBackInBoundsToleranceX())
   )
   {
      // back within close boundaries
      _focus_x_triggered = false;
   }

   if (_focus_x_triggered || room_corrected)
   {
      _x += dx;
   }
}


void CameraSystem::updateY(const sf::Time& dt)
{
   auto& camera_config = CameraSystemConfiguration::getInstance();

   const auto p_range  = _view_height / camera_config.getPanicLineDivider();
   const auto p_center = _view_height / 2.0f;

   _panic_line_y0 = p_center - p_range;
   _panic_line_y1 = p_center + p_range;

   const auto view_center = (_view_height / 2.0f);

   // test if out of panic line boundaries
   auto player = Player::getCurrent();
   auto player_x = player->getPixelPositionf().x;
   auto player_y = player->getPixelPositionf().y + camera_config.getPlayerOffsetY();
   const auto room_corrected = CameraRoomLock::correctedCamera(player_x, player_y, _focus_offset);
   const auto test = player_y - view_center;
   const auto p0 = _y - _panic_line_y1;
   const auto p1 = _y - _panic_line_y0;

   // if once panicking, keep following the player in panic mode until he's back on the ground
   if (_panic)
   {
      if (!player->isInAir())
      {
         _panic = false;
      }

      _focus_y_triggered = true;
   }
   else
   {
      _panic = (test < p0 || test + player->getPlayerPixelRect().height > p1);
   }

   // test if back within close boundaries
   if (
         (test > _y - view_center - camera_config.getBackInBoundsToleranceY())
      && (test < _y - view_center + camera_config.getBackInBoundsToleranceY())
   )
   {
      _focus_y_triggered = false;
   }

   if (player->isInAir() && !_focus_y_triggered &&! room_corrected)
   {
      _no_y_update_triggered = false;
      return;
   }

   if (!_no_y_update_triggered)
   {
      // reset y camera acceleration
      _no_y_update_triggered = true;
      _y_update_start_time = sf::Time{};
   }

   _y_update_start_time += dt;

   // have some acceleration in the y update velocity so it doesn't got at full speed instantly
   const auto y_update_start_time_s = _y_update_start_time.asSeconds();
   const auto y_update_acceleration =
      _panic
         ? 2.0f
         : std::min(Easings::easeOutQuint(y_update_start_time_s), 1.0f);

   const auto dy = (player_y - _y) * dt.asSeconds() * camera_config.getCameraVelocityFactorY() * y_update_acceleration;

   _y += dy;
}


float CameraSystem::getFocusZoneCenter() const
{
   return _focus_zone_center;
}


float CameraSystem::getFocusOffset() const
{
   return _focus_offset;
}


void CameraSystem::syncNow()
{
   auto player = Player::getCurrent();

   auto player_x = player->getPixelPositionf().x;
   auto player_y = player->getPixelPositionf().y;

   CameraRoomLock::correctedCamera(player_x, player_y, _focus_offset);

   _x = player_x;
   _y = player_y;
}


float CameraSystem::getX() const
{
   // camera should be in the center of the focus zone
   return _x - _focus_zone_center;
}


float CameraSystem::getY() const
{
   return _y - (_view_height / CameraSystemConfiguration::getInstance().getViewRatioY());
}


float CameraSystem::getFocusZoneX0() const
{
   return _focus_zone_x0;
}


float CameraSystem::getFocusZoneX1() const
{
   return _focus_zone_x1;
}


CameraSystem& CameraSystem::getCameraSystem()
{
   static CameraSystem __instance;
   return __instance;
}


float CameraSystem::getPanicLineY0() const
{
   return _panic_line_y0;
}


float CameraSystem::getPanicLineY1() const
{
   return _panic_line_y1;
}


