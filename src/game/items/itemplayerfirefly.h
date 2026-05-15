#pragma once

#include "game/items/item.h"

/// \brief inventory item that activates or deactivates the player firefly companion mechanism.
class ItemPlayerFirefly : public Item
{
public:
   /// \brief enables the player firefly mechanism when equipped.
   void onEquipped() override;

   /// \brief disables the player firefly mechanism when unequipped.
   void onUnequipped() override;

   /// \brief gets the inventory display name for this item.
   /// \return the string "PlayerFirefly".
   std::string getName() const override;
};
