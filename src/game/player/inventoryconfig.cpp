#include "inventoryconfig.h"

#include "game/player/inventory.h"
#include "game/player/itemsystem.h"
#include "game/state/savestate.h"

namespace InventoryConfig
{
void setupItemSystemCallbacks(Inventory& inventory)
{
   // add ItemSystem callback to sync items with inventory
   inventory._added_callbacks.push_back(
      [](const std::string& item_name) { SaveState::getPlayerInfo()._items.onInventoryItemAdded(item_name); }
   );
   inventory._removed_callbacks.push_back(
      [](const std::string& item_name) { SaveState::getPlayerInfo()._items.onInventoryItemRemoved(item_name); }
   );
   inventory._updated_callbacks.push_back([]()
                                          { SaveState::getPlayerInfo()._items.syncWithInventory(SaveState::getPlayerInfo()._inventory._slots); });

   // initial sync of items
   for (const auto& item_name : inventory._items)
   {
      SaveState::getPlayerInfo()._items.onInventoryItemAdded(item_name);
   }
   SaveState::getPlayerInfo()._items.syncWithInventory(inventory._slots);
}
}  // namespace InventoryConfig
