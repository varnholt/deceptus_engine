#pragma once

#include <memory>
#include <string>
#include <vector>

#include "extratable.h"
#include "inventory.h"


class PlayerInfo
{
   public:

      Inventory mInventory;
      ExtraTable mExtraTable;
      std::string mName;
};

