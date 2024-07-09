#ifndef INVENTORYBASEDCONTROLS_H
#define INVENTORYBASEDCONTROLS_H

#include "game/player/inventory.h"

namespace InventoryBasedControls
{
bool isSwordButtonPressed(const Inventory& inventory, bool x_button_pressed, bool y_button_pressed);
bool isAttackButtonPressed(const Inventory& inventory, bool x_button_pressed, bool y_button_pressed);
};

#endif  // INVENTORYBASEDCONTROLS_H
