#ifndef MENUCONFIG_H
#define MENUCONFIG_H

#include <array>
#include <chrono>

/// \brief stores configurable show and hide durations for in-game menu animations.
struct MenuConfig
{
   /// \brief loads menu timing values from data/config/menus.json.
   MenuConfig();

   using FloatSeconds = std::chrono::duration<float>;

   /// \brief positions of the inventory menu elements that are not psd layers.
   ///
   /// the two slot badges and the two equip hints are sprites into the ui icon atlas, drawn over the
   /// button plates that the psd paints. they are placed by their atlas cell rather than by their
   /// visible pixels: a small icon carries its artwork at cell offset 5, 4 and a large one at 3, 3.
   struct InventoryLayout
   {
      std::array<float, 2> _slot_badge_x_px{65.0f, 113.0f};  //!< small icons, on the profile panel slots
      float _slot_badge_y_px{138.0f};
      std::array<float, 2> _equip_hint_x_px{543.0f, 570.0f};  //!< large icons, next to the equip label
      float _equip_hint_y_px{246.0f};
   };

   FloatSeconds _duration_show;
   FloatSeconds _duration_hide;
   InventoryLayout _inventory;
};

#endif  // MENUCONFIG_H
