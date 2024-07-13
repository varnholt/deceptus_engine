#include "inventorybasedcontrols.h"

constexpr std::string_view sword_identifier{"sword"};

bool InventoryBasedControls::isSwordButtonPressed(const Inventory& inventory, bool x_button_pressed, bool y_button_pressed)
{
   const auto pressed =
      (x_button_pressed && inventory._slots[0] == sword_identifier) || (y_button_pressed && inventory._slots[1] == sword_identifier);

   return pressed;
}

bool InventoryBasedControls::isAttackButtonPressed(const Inventory& inventory, bool x_button_pressed, bool y_button_pressed)
{
   // for now there's just the sword
   return isSwordButtonPressed(inventory, x_button_pressed, y_button_pressed);
}
