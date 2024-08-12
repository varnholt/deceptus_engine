#ifndef ROOMUPDATER_H
#define ROOMUPDATER_H

#include <memory>
#include <optional>
#include <string>
#include <vector>

struct Room;

namespace RoomUpdater
{
bool checkCurrentMatchesId(int32_t);
bool checkCurrentMatchesIds(const std::vector<int32_t>& ids);
void setCurrent(const std::shared_ptr<Room>& current);
void setPrevious(const std::shared_ptr<Room>& previous);
std::optional<int32_t> getCurrentId();
std::optional<int32_t> getPreviousId();
std::shared_ptr<Room> getCurrent();
std::shared_ptr<Room> getPrevious();
std::string getCurrentRoomName();
void setSynced(bool synced);
bool isSynced();
};  // namespace RoomUpdater

#endif  // ROOMUPDATER_H
