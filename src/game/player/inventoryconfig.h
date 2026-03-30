#pragma once

#include "player/inventory.h"

#include <functional>

struct Inventory;
struct ItemSystem;

/// \brief owns callback bindings that connect inventory events to the item system.
struct InventoryConfig
{
   // stored callbacks for removal before re-registering
   Inventory::AddedCallback _added_callback;
   Inventory::RemovedCallback _removed_callback;
   Inventory::UpdatedCallback _updated_callback;

   // link inventory to itemsystem by registering callbacks
   // call this after any inventory instance is created or replaced
   /// \brief rebinds inventory callbacks and synchronizes current inventory state into the item system.
   /// \param inventory inventory instance whose callbacks are registered and cleaned up.
   /// \param item_system item system that receives add, remove, and slot-sync notifications.
   void linkInventoryToItemSystem(Inventory& inventory, ItemSystem& item_system);
};
