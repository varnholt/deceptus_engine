#pragma once

#include "inventoryitem.h"

#include "json/json.hpp"


struct Inventory
{
   Inventory() = default;

   void add(ItemType);
   bool hasInventoryItem(ItemType itemType) const;
   const std::vector<InventoryItem>& getItems() const;
   void clear();

   void resetKeys();
   void giveAllKeys();

   std::vector<InventoryItem> _items;
};


void to_json(nlohmann::json& j, const Inventory& d);
void from_json(const nlohmann::json& j, Inventory& d);

