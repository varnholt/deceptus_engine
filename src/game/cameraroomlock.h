#pragma once

#include <memory>
#include "room.h"

namespace CameraRoomLock
{
bool correctedCamera(float& x, float& y, float focus_offset);
void setRoom(const std::shared_ptr<Room>& room);
void readLockedSides(bool& left, bool& right, bool& top, bool& bottom);
}

