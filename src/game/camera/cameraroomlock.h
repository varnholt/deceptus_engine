#pragma once

#include <array>
#include <memory>
#include <optional>
#include "game/level/room.h"

namespace CameraRoomLock
{

/// \brief the range a camera center may take without the view reaching outside the room.
struct HorizontalLimits
{
   float _left_px{0.0f};   //!< leftmost camera center that still keeps the view inside the room
   float _right_px{0.0f};  //!< rightmost camera center that still keeps the view inside the room
};

/// \brief returns the horizontal range the camera center may take inside the player's sub-room.
/// \param player_position_px player position, used to pick the sub-room to measure against.
/// \param focus_offset horizontal focus-zone shift applied by the camera system.
/// \return the range, or nothing when there is no room or no sub-room holding the player.
/// \note deliberately looked up at the player rather than at the position being limited. The point
///       being limited is by definition the one that may already be outside, and a lookup there finds
///       no sub-room and would report no limits at all - exactly when they are needed.
std::optional<HorizontalLimits> horizontalCameraLimits(const sf::Vector2f& player_position_px, float focus_offset);

/// \brief checks whether the current camera view extends beyond the active sub-room bounds.
/// \return boundary flags ordered as up, down, left, right; true means the view crosses that side.
std::array<bool, 4> checkRoomBoundaries();

/// \brief clamps a camera center candidate to the current sub-room while respecting focus offset.
/// \param x camera center x in pixels; overwritten when horizontal clamping is needed.
/// \param y camera center y in pixels; overwritten when vertical clamping is needed.
/// \param focus_offset horizontal focus-zone shift applied by the camera system.
/// \return true when at least one side is clamped against a room boundary.
bool correctedCamera(float& x, float& y, float focus_offset);

/// \brief reads the last lock state computed by correctedCamera().
/// \param left receives whether the camera was clamped at the left boundary.
/// \param right receives whether the camera was clamped at the right boundary.
/// \param top receives whether the camera was clamped at the top boundary.
/// \param bottom receives whether the camera was clamped at the bottom boundary.
void readLockedSides(bool& left, bool& right, bool& top, bool& bottom);

/// \brief sets the room whose sub-room rectangles are used for camera boundary checks.
/// \param room active room instance used for boundary lookup.
void setRoom(const std::shared_ptr<Room>& room);

/// \brief updates the current camera view rectangle used by boundary probing.
/// \param rect current view rectangle in world pixel coordinates.
void setViewRect(const sf::FloatRect& rect);

/// \brief returns the view rectangle the level last computed, in world pixel coordinates.
///
/// Level::updateViews publishes it here at the top of every simulation step, so anything updating
/// later in the same step can ask what the camera is about to show. That is not the same as the
/// rectangle the frame is drawn through - draw uses the interpolated camera - so a caller deciding
/// whether to skip work needs a margin rather than treating this as the exact visible region.
const sf::FloatRect& getViewRect();
}  // namespace CameraRoomLock
