#pragma once

#include "game/player/playerinterface.h"

#include <memory>
#include <vector>

/// \brief global registry of active player instances; replaces the Player singleton pattern
///        and enables future support for multiple simultaneous players.
class PlayerRegistry
{
public:
   /// \brief registers a player instance; called from game after constructing the player.
   /// \param player shared ownership of the player to register.
   static void add(std::shared_ptr<PlayerInterface> player);

   /// \brief unregisters a player instance; called from the Player destructor.
   /// \param player raw pointer used to identify the instance to remove.
   static void remove(PlayerInterface* player);

   /// \brief returns the first registered player, equivalent to the former singleton.
   /// \return shared pointer to the first player, or nullptr when no player is registered.
   static std::shared_ptr<PlayerInterface> getFirst();

   /// \brief returns all currently registered players.
   /// \return read-only view of the player list.
   static const std::vector<std::shared_ptr<PlayerInterface>>& getAll();
};
