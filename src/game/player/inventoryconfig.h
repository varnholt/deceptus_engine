#pragma once

#include <functional>

struct Inventory;

namespace InventoryConfig
{
void setupItemSystemCallbacks(Inventory& inventory);
}  // namespace InventoryConfig
