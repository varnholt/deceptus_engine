#pragma once

#include "inventoryitem.h"


class Inventory
{
   public:

      Inventory() = default;

      void add(ItemType);
      bool hasInventoryItem(ItemType itemType) const;
      const std::vector<InventoryItem>& getItems() const;
      void clear();

      void resetKeys();
      void giveAllKeys();


   private:

      std::vector<InventoryItem> mItems;
};

