#pragma once

#include "inventoryitem.h"


class Inventory
{
   public:
      Inventory() = default;
      void add(ItemType);
      void clear();
      void resetKeys();
      void giveAllKeys();
      bool hasInventoryItem(ItemType itemType) const;

      const std::vector<InventoryItem>& getItems() const;

   private:
      std::vector<InventoryItem> mItems;
};

