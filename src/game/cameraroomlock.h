#pragma once

#include <array>
#include <memory>
#include "room.h"

namespace CameraRoomLock
{
std::array<bool, 4> checkRoomBoundaries();
bool correctedCamera(float& x, float& y, float focus_offset);
void readLockedSides(bool& left, bool& right, bool& top, bool& bottom);
void setRoom(const std::shared_ptr<Room>& room);
void setViewRect(const sf::FloatRect& rect);
}

