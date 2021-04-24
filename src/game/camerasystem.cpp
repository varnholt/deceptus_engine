#include "camerasystem.h"

#include "framework/easings/easings.h"
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


CameraSystem CameraSystem::sInstance;


void CameraSystem::update(const sf::Time& dt, float viewWidth, float viewHeight)
{
   _view_width = viewWidth;
   _view_height = viewHeight;

   updateX(dt);
   updateY(dt);
}


void CameraSystem::updateX(const sf::Time& dt)
{
   auto player = Player::getCurrent();
   auto& config = CameraSystemConfiguration::getInstance();

   auto player_x = player->getPixelPositionf().x;
   auto player_y = player->getPixelPositionf().y;
   const auto corrected = _room->correctedCamera(player_x, player_y, _focus_offset, config.getViewRatioY());

   const auto dx = (player_x - _x) * dt.asSeconds() * config.getCameraVelocityFactorX();

   const auto f_center = _view_width / 2.0f;
   const auto f_range  = _view_width / config.getFocusZoneDivider();

   _focus_zone_x0 = f_center - f_range;
   _focus_zone_x1 = f_center + f_range;

   // shift focus zone based on player orientation
   const auto target_offset =
      player->isPointingLeft()
         ? ( f_range * config.getTargetShiftFactor())
         : (-f_range * config.getTargetShiftFactor());

   const auto fcd = (target_offset - _focus_offset) * dt.asSeconds() * config.getCameraVelocityFactorX();
   if (fabs(_focus_offset) < fabs(f_range * config.getTargetShiftFactor()))
   {
      _focus_offset += fcd;
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

   // test if back within close boundaries
   else if (
         (test > _x - _focus_zone_center - config.getBackInBoundsToleranceX())
      && (test < _x - _focus_zone_center + config.getBackInBoundsToleranceX())
   )
   {
      _focus_x_triggered = false;
   }

   if (_focus_x_triggered || corrected)
   {
      _x += dx;
   }
}


void CameraSystem::updateY(const sf::Time& dt)
{
   auto& config = CameraSystemConfiguration::getInstance();

   const auto p_range  = _view_height / config.getPanicLineDivider();
   const auto p_center = _view_height / 2.0f;

   _panic_line_y0 = p_center - p_range;
   _panic_line_y1 = p_center + p_range;

   const auto view_center = (_view_height / 2.0f);

   // test if out of panic line boundaries
   auto player = Player::getCurrent();

   auto player_x = player->getPixelPositionf().x;
   auto player_y = player->getPixelPositionf().y + config.getPlayerOffsetY();
   const auto corrected = _room->correctedCamera(player_x, player_y, _focus_offset, config.getViewRatioY());

   const auto test = player_y - view_center;

   const auto p0 = _y - _panic_line_y1;
   const auto p1 = _y - _panic_line_y0;

   if ((test < p0 || test > p1) /*|| !player->isInAir()*/)
   {
      _focus_y_triggered = true;
   }

   // test if back within close boundaries
   else if (
         (test > _y - view_center - config.getBackInBoundsToleranceY())
      && (test < _y - view_center + config.getBackInBoundsToleranceY())
   )
   {
      _focus_y_triggered = false;
   }

   if (player->isInAir() && !_focus_y_triggered &&! corrected)
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
   const auto y_update_acceleration = std::min(Easings::easeOutQuint(y_update_start_time_s), 1.0f);

   const auto dy = (player_y - _y) * dt.asSeconds() * config.getCameraVelocityFactorY() * y_update_acceleration;

   _y += dy;
}


void CameraSystem::setRoom(const std::optional<Room>& room)
{
   if (_room->mId != room->mId)
   {
      _room_interpolation = 0.0f;
      // std::cout << "[i] reset room interpolation" << std::endl;
   }

   _room = room;
}


void CameraSystem::syncNow()
{
   auto player = Player::getCurrent();
   _x = player->getPixelPositionf().x;
   _y = player->getPixelPositionf().y;
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
   return sInstance;
}


float CameraSystem::getPanicLineY0() const
{
   return _panic_line_y0;
}


float CameraSystem::getPanicLineY1() const
{
   return _panic_line_y1;
}


