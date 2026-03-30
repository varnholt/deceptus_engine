#pragma once

#include <array>
#include <memory>
#include "game/level/room.h"

namespace CameraRoomLock
{
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
}  // namespace CameraRoomLock
