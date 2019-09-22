#pragma once

#include "constants.h"
#include <SFML/Graphics.hpp>

struct InventoryItem
{
   InventoryItem() = default;

   InventoryItem(ItemType);

   ItemType mType = ItemType::Invalid;
};

