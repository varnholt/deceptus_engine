#pragma once

#include <memory>
#include "room.h"

class CameraRoomLock
{

public:
   bool correctedCamera(float& x, float& y, float focus_offset, float view_ratio_y) const;
   void setRoom(const std::shared_ptr<Room>& room);
   static CameraRoomLock& instance();

private:
   CameraRoomLock() = default;
   std::shared_ptr<Room> _room;

};

