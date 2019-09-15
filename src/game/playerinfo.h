#pragma once

#include <memory>
#include <vector>

#include "extratable.h"
#include "inventoryitem.h"


class PlayerInfo
{
   public:

      std::vector<std::shared_ptr<InventoryItem>> mInventory;
      ExtraTable mExtraTable;

      static PlayerInfo& getCurrent();


   private:

      PlayerInfo() = default;
      static PlayerInfo sCurrent;
};

