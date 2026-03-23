#include "inventoryconfig.h"

#include "game/player/inventory.h"
#include "game/player/itemsystem.h"
#include "game/state/savestate.h"

#include <algorithm>

void InventoryConfig::linkInventoryToItemSystem(Inventory& inventory, ItemSystem& item_system)
{
   // remove existing callbacks if they have targets (i.e., already registered)
   if (_added_callback.target<void(const std::string&)>() != nullptr)
   {
      inventory.removeAddedCallback(_added_callback);
   }

   if (_removed_callback.target<void(const std::string&)>() != nullptr)
   {
      inventory.removeRemovedCallback(_removed_callback);
   }

   if (_updated_callback.target<void()>() != nullptr)
   {
      inventory.removeUpdatedCallback(_updated_callback);
   }

   // create new callbacks
   _added_callback = [&item_system](const std::string& item_name) { item_system.onInventoryItemAdded(item_name); };
   _removed_callback = [&item_system](const std::string& item_name) { item_system.onInventoryItemRemoved(item_name); };
   _updated_callback = [&item_system]() { item_system.syncInventorySlots(SaveState::getPlayerInfo()._inventory._slots); };

   // register callbacks
   inventory._added_callbacks.push_back(_added_callback);
   inventory._removed_callbacks.push_back(_removed_callback);
   inventory._updated_callbacks.push_back(_updated_callback);

   // initial sync of items
   for (const auto& item_name : inventory._items)
   {
      item_system.onInventoryItemAdded(item_name);
   }

   item_system.syncInventorySlots(inventory._slots);
}
