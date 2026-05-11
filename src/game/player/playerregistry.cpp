#include "playerregistry.h"

#include <ranges>

namespace
{
std::vector<std::shared_ptr<PlayerInterface>> registered_players;
}

void PlayerRegistry::add(std::shared_ptr<PlayerInterface> player)
{
   registered_players.push_back(std::move(player));
}

void PlayerRegistry::remove(PlayerInterface* player)
{
   std::erase_if(registered_players, [player](const auto& stored) { return stored.get() == player; });
}

std::shared_ptr<PlayerInterface> PlayerRegistry::getFirst()
{
   if (registered_players.empty())
   {
      return nullptr;
   }
   return registered_players.front();
}

const std::vector<std::shared_ptr<PlayerInterface>>& PlayerRegistry::getAll()
{
   return registered_players;
}
