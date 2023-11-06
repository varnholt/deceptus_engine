#include "roomupdater.h"

#include "game/room.h"

namespace
{
std::shared_ptr<Room> _room_current;
std::shared_ptr<Room> _room_previous;
bool _room_synced = false;
}  // namespace

void RoomUpdater::setSynced(bool synced)
{
   _room_synced = synced;
}

bool RoomUpdater::isSynced()
{
   return _room_synced;
}

std::shared_ptr<Room> RoomUpdater::getCurrent()
{
   return _room_current;
}

std::shared_ptr<Room> RoomUpdater::getPrevious()
{
   return _room_previous;
}

void RoomUpdater::setPrevious(const std::shared_ptr<Room>& previous)
{
   _room_previous = previous;
}

void RoomUpdater::setCurrent(const std::shared_ptr<Room>& current)
{
   _room_current = current;
}

std::optional<int32_t> RoomUpdater::getCurrentId()
{
   if (!_room_current)
   {
      return std::nullopt;
   }

   return _room_current->_id;
}

std::optional<int32_t> RoomUpdater::getPreviousId()
{
   if (!_room_previous)
   {
      return std::nullopt;
   }

   return _room_previous->_id;
}

bool RoomUpdater::checkCurrentMatchesId(int32_t value)
{
   if (!_room_current)
   {
      return false;
   }

   return value == _room_current->_id;
}

bool RoomUpdater::checkCurrentMatchesIds(const std::vector<int32_t>& ids)
{
   if (!_room_current)
   {
      return false;
   }

   return std::find(ids.cbegin(), ids.cend(), _room_current->_id) != ids.cend();
}

std::string RoomUpdater::getCurrentRoomName()
{
   if (!_room_current)
   {
      return "undefined";
   }

   return _room_current->getObjectId();
}
