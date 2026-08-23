#include "camerasystem.h"

#include "framework/easings/easings.h"
#include "game/camera/cameraroomlock.h"
#include "game/camera/camerasystemconfiguration.h"
#include "game/player/playerregistry.h"

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

namespace
{
//!< how much faster the lead's speed collapses than it builds. A player slows down faster than the
//!< catch-up can follow at its own rate, and an aim point still carrying the old speed would sit
//!< ahead of one who has already stopped
constexpr auto lead_decay_factor = 3.0f;
}  // namespace

void CameraSystem::update(const sf::Time& dt, float view_width_px, float view_height_px)
{
   // where the camera stood going into this step, so the frames drawn before the next one can be
   // placed between the two
   _previous_x_px = getX();
   _previous_y_px = getY();

   _view_width_px = view_width_px;
   _view_height_px = view_height_px;

   if (_locked)
   {
      return;
   }

   updateX(dt);
   updateY(dt);
}

void CameraSystem::updateX(const sf::Time& delta_time)
{
   const auto& camera_config = CameraSystemConfiguration::getInstance();

   auto player = PlayerRegistry::getFirst();
   const auto player_position_px = player->getPixelPositionFloat();
   const auto velocity_factor = camera_config.getCameraVelocityFactorX();

   // how fast the player is moving, in the camera's own units and time base. Read off two positions
   // rather than taken from the physics body on purpose: the box2d delta is not the step duration, see
   // FixedTimeStep, so a velocity in box2d units would need a conversion that has nothing to do with
   // the camera
   const auto player_velocity_px_per_s = (player_position_px.x - _previous_player_x_px) / delta_time.asSeconds();
   _previous_player_x_px = player_position_px.x;

   // the aim point is carried ahead of the player by the distance the camera needs in order to travel
   // at the player's speed. The camera closes a fraction of its distance to the aim point every step,
   // so its speed *is* that distance times the factor - aim straight at the player and the camera is
   // left behind by exactly that distance for as long as the player keeps moving.
   //
   // The lead is built from a speed that takes a moment to catch up, and that is where the camera's own
   // acceleration comes from. Leading by the full distance against the *current* speed would cancel the
   // follow at every speed alike: no lag left, but no easing either, and the camera is welded to the
   // player. Letting the lead's speed catch up at the same rate the camera does puts the easing back
   // where it belongs - while the player is speeding up or slowing down the lead is still short and the
   // camera eases towards the new speed, and once the speed is steady the lead is the whole distance
   // and the camera sits on the player with nothing left over
   //
   // The catch-up runs at that rate only while the player is speeding up. Slowing down, the lead
   // collapses faster, so the aim point stays behind a player who has stopped rather than hanging on
   // ahead of them and having to come back. Collapsing it faster rather than snapping it to their
   // current speed is what keeps the follow's ease out: the camera still glides to a halt instead of
   // running out of distance to cover all at once
   const auto catch_up_rate =
      (fabs(player_velocity_px_per_s) >= fabs(_lead_velocity_px_per_s)) ? velocity_factor : (velocity_factor * lead_decay_factor);

   _lead_velocity_px_per_s += (player_velocity_px_per_s - _lead_velocity_px_per_s) * delta_time.asSeconds() * catch_up_rate;

   const auto lead_px =
      (velocity_factor > 0.0f) ? (_lead_velocity_px_per_s * camera_config.getCameraLeadFactorX() / velocity_factor) : 0.0f;

   // the room lock still supplies the y target and reports whether any side is clamped, unchanged. It
   // is handed the player's own position, never a led one: it looks the player up in a sub-room to find
   // the rectangle to work against, and a lead can push that lookup outside the room, where it finds
   // nothing and skips the clamp altogether
   auto corrected_x_px = player_position_px.x;
   auto corrected_y_px = player_position_px.y;
   const auto room_corrected = CameraRoomLock::correctedCamera(corrected_x_px, corrected_y_px, _focus_offset_px);

   // holding the aim point inside the room is what shapes both ends of a room crossing, and it does it
   // without a special case at either end. Leaving a clamp, the aim point comes off the wall while the
   // player is still a lead short of it, so the camera is already up to speed by the time the player
   // passes - instead of standing on the clamp with nothing to move towards and needing the better part
   // of a second to get going. Arriving at the far wall, the aim point reaches it a lead early, so the
   // camera runs out of distance and decelerates into it
   auto target_x_px = player_position_px.x + lead_px;
   const auto limits = CameraRoomLock::horizontalCameraLimits(player_position_px, _focus_offset_px);
   if (limits.has_value())
   {
      // a room narrower than the view cannot satisfy both ends at once, and correctedCamera settles
      // that in favour of the left one, so settle it the same way here
      target_x_px = std::max(limits->_left_px, std::min(target_x_px, limits->_right_px));
   }

   _dx_px = (target_x_px - _x_px);
   const auto dx_px = (_dx_px)*delta_time.asSeconds() * velocity_factor;
   const auto f_center = _view_width_px / 2.0f;
   const auto f_range = _view_width_px / camera_config.getFocusZoneDivider();

   _focus_zone_x0_px = f_center - f_range;
   _focus_zone_x1_px = f_center + f_range;

   // shift focus zone based on player orientation
   auto target_offset = 0.0f;
   if (camera_config.isFollowingPlayerOrientation())
   {
      target_offset =
         player->isPointingLeft() ? (f_range * camera_config.getTargetShiftFactor()) : (-f_range * camera_config.getTargetShiftFactor());
   }

   const auto focus_delta = (target_offset - _focus_offset_px) * delta_time.asSeconds() * camera_config.getCameraVelocityFactorX();
   if (fabs(_focus_offset_px) < fabs(f_range * camera_config.getTargetShiftFactor()))
   {
      _focus_offset_px += focus_delta;
   }

   _focus_zone_x0_px += _focus_offset_px;
   _focus_zone_x1_px += _focus_offset_px;
   _focus_zone_center_px = ((_focus_zone_x0_px + _focus_zone_x1_px) / 2.0f);

   // test if out of focus zone boundaries. Measured against the point the camera is aiming at rather
   // than the player: now that the lead cancels the trailing distance, a running player sits on the
   // centre, so testing the player would find them inside the zone and stop the camera every few
   // frames - the camera would judder along instead of running with them
   const auto test = target_x_px - _focus_zone_center_px;
   const auto f0_px = _x_px - _focus_zone_x1_px;
   const auto f1_px = _x_px - _focus_zone_x0_px;

   if (test < f0_px || test > f1_px)
   {
      _focus_x_triggered = true;
   }
   else if ((test > _x_px - _focus_zone_center_px - camera_config.getBackInBoundsToleranceX()) &&
            (test < _x_px - _focus_zone_center_px + camera_config.getBackInBoundsToleranceX()))
   {
      // back within close boundaries
      _focus_x_triggered = false;
   }

   if (_focus_x_triggered || room_corrected)
   {
      _x_px += dx_px;
   }

   // and the same limits on the camera itself, not only on what it is aiming at. A room can change
   // under a camera that is standing still, and the focus zone gate can hold it where it is, so being
   // aimed inside the room is not on its own enough to have never been outside it
   if (limits.has_value())
   {
      _x_px = std::max(limits->_left_px, std::min(_x_px, limits->_right_px));
   }
}

void CameraSystem::updateY(const sf::Time& delta_time)
{
   const auto& camera_config = CameraSystemConfiguration::getInstance();

   const auto p_range = _view_height_px / camera_config.getPanicLineDivider();
   const auto p_center = _view_height_px / 2.0f;

   _panic_line_y0_px = p_center - p_range;
   _panic_line_y1_px = p_center + p_range;

   const auto view_center = (_view_height_px / 2.0f);

   // test if out of panic line boundaries
   auto player = PlayerRegistry::getFirst();
   auto player_x = player->getPixelPositionFloat().x;
   auto player_y = player->getPixelPositionFloat().y + camera_config.getPlayerOffsetY();
   const auto room_corrected = CameraRoomLock::correctedCamera(player_x, player_y, _focus_offset_px);
   const auto test = player_y - view_center;
   const auto p0 = _y_px - _panic_line_y1_px;
   const auto p1 = _y_px - _panic_line_y0_px;

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
      _panic = (test < p0 || test + player->getPixelRectInt().size.y > p1);
   }

   // test if back within close boundaries
   if ((test > _y_px - view_center - camera_config.getBackInBoundsToleranceY()) &&
       (test < _y_px - view_center + camera_config.getBackInBoundsToleranceY()))
   {
      _focus_y_triggered = false;
   }

   if (player->isInAir() && !_focus_y_triggered && !room_corrected)
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

   _y_update_start_time += delta_time;

   // have some acceleration in the y update velocity so it doesn't got at full speed instantly
   const auto y_update_start_time_s = _y_update_start_time.asSeconds();
   const auto y_update_acceleration =
      _panic ? camera_config.getPanicAccelerationFactorY() : std::min(Easings::easeOutQuint(y_update_start_time_s), 1.0f);

   _dy_px = player_y - _y_px;
   const auto dy = _dy_px * delta_time.asSeconds() * camera_config.getCameraVelocityFactorY() * y_update_acceleration;

   _y_px += dy;
}

float CameraSystem::getDy() const
{
   return _dy_px;
}

float CameraSystem::getDx() const
{
   return _dx_px;
}

float CameraSystem::getFocusZoneCenter() const
{
   return _focus_zone_center_px;
}

float CameraSystem::getFocusOffset() const
{
   return _focus_offset_px;
}

void CameraSystem::syncNow()
{
   auto player = PlayerRegistry::getFirst();

   auto player_x = player->getPixelPositionFloat().x;
   auto player_y = player->getPixelPositionFloat().y;

   CameraRoomLock::correctedCamera(player_x, player_y, _focus_offset_px);

   _x_px = player_x;
   _y_px = player_y;

   // a teleport is not movement. Left alone, the jump would read as an enormous speed for one step and
   // the lead would fling the camera across the room
   _previous_player_x_px = player->getPixelPositionFloat().x;
   _lead_velocity_px_per_s = 0.0f;
}

void CameraSystem::snapTo(float x_px, float y_px)
{
   _x_px = x_px;
   _y_px = y_px;
   _dx_px = 0.0f;
   _dy_px = 0.0f;
   _locked = true;
   _previous_player_x_px = PlayerRegistry::getFirst()->getPixelPositionFloat().x;
   _lead_velocity_px_per_s = 0.0f;
}

void CameraSystem::unlockCamera()
{
   _locked = false;
}

sf::Vector2f CameraSystem::getCenterPx() const
{
   return {_x_px, _y_px};
}

float CameraSystem::getX() const
{
   // camera should be in the center of the focus zone
   return _x_px - _focus_zone_center_px;
}

float CameraSystem::getY() const
{
   return _y_px - (_view_height_px / CameraSystemConfiguration::getInstance().getViewRatioY());
}

float CameraSystem::getPreviousX() const
{
   return _previous_x_px;
}

float CameraSystem::getPreviousY() const
{
   return _previous_y_px;
}

float CameraSystem::getFocusZoneX0() const
{
   return _focus_zone_x0_px;
}

float CameraSystem::getFocusZoneX1() const
{
   return _focus_zone_x1_px;
}

CameraSystem& CameraSystem::getInstance()
{
   static CameraSystem instance;
   return instance;
}

float CameraSystem::getPanicLineY0() const
{
   return _panic_line_y0_px;
}

float CameraSystem::getPanicLineY1() const
{
   return _panic_line_y1_px;
}
