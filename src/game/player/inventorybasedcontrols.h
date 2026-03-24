#ifndef INVENTORYBASEDCONTROLS_H
#define INVENTORYBASEDCONTROLS_H

#include "game/player/inventory.h"

namespace InventoryBasedControls
{
/// \brief checks whether the pressed face button maps to a sword-equipped slot.
/// \param inventory inventory whose slot 0 maps to x and slot 1 maps to y.
/// \param x_button_pressed true when the x action button is pressed.
/// \param y_button_pressed true when the y action button is pressed.
/// \return true when the pressed button corresponds to a slot containing "sword".
bool isSwordButtonPressed(const Inventory& inventory, bool x_button_pressed, bool y_button_pressed);

/// \brief checks whether any currently supported attack input is pressed.
/// \param inventory inventory used to resolve equipped items per button slot.
/// \param x_button_pressed true when the x action button is pressed.
/// \param y_button_pressed true when the y action button is pressed.
/// \return true when the current attack mapping is active; currently identical to sword input.
bool isAttackButtonPressed(const Inventory& inventory, bool x_button_pressed, bool y_button_pressed);
};  // namespace InventoryBasedControls

#endif  // INVENTORYBASEDCONTROLS_H
