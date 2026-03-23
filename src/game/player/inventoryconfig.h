#pragma once

#include "player/inventory.h"

#include <functional>

struct Inventory;
struct ItemSystem;

struct InventoryConfig
{
   // stored callbacks for removal before re-registering
   Inventory::AddedCallback _added_callback;
   Inventory::RemovedCallback _removed_callback;
   Inventory::UpdatedCallback _updated_callback;

   // link inventory to itemsystem by registering callbacks
   // call this after any inventory instance is created or replaced
   void linkInventoryToItemSystem(Inventory& inventory, ItemSystem& item_system);
};
