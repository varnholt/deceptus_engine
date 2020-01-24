#pragma once

#include "constants.h"
#include <SFML/Graphics.hpp>

#include "json/json.hpp"

struct InventoryItem
{
   InventoryItem() = default;

   InventoryItem(ItemType);

   ItemType mType = ItemType::Invalid;
};


void to_json(nlohmann::json& j, const InventoryItem& d);
void from_json(const nlohmann::json& j, InventoryItem& d);

