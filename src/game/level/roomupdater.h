#ifndef ROOMUPDATER_H
#define ROOMUPDATER_H

#include <memory>
#include <optional>
#include <string>
#include <vector>

struct Room;

namespace RoomUpdater
{
/// \brief checks whether the current room id matches a single id.
/// \param value room id to compare against the current room.
/// \return true when a current room exists and its id equals \p value.
bool checkCurrentMatchesId(int32_t);
/// \brief checks whether the current room id is contained in a list of ids.
/// \param ids room ids to test.
/// \return true when a current room exists and its id is present in \p ids.
bool checkCurrentMatchesIds(const std::vector<int32_t>& ids);
/// \brief stores the current active room reference.
/// \param current room to store as current.
void setCurrent(const std::shared_ptr<Room>& current);
/// \brief stores the previous room reference.
/// \param previous room to store as previous.
void setPrevious(const std::shared_ptr<Room>& previous);
/// \brief gets the id of the current room.
/// \return current room id, or std::nullopt when no current room is set.
std::optional<int32_t> getCurrentId();
/// \brief gets the id of the previous room.
/// \return previous room id, or std::nullopt when no previous room is set.
std::optional<int32_t> getPreviousId();
/// \brief gets the current room reference.
/// \return shared pointer to the current room, or nullptr.
std::shared_ptr<Room> getCurrent();
/// \brief gets the previous room reference.
/// \return shared pointer to the previous room, or nullptr.
std::shared_ptr<Room> getPrevious();
/// \brief gets the object id string of the current room.
/// \return current room object id, or "undefined" when no current room exists.
std::string getCurrentRoomName();
/// \brief marks whether camera/room synchronization has completed.
/// \param synced synchronization state to store.
void setSynced(bool synced);
/// \brief reads the synchronization state flag.
/// \return true when room synchronization is marked as complete.
bool isSynced();
};  // namespace RoomUpdater

#endif  // ROOMUPDATER_H
