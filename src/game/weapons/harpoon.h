#pragma once

#include "weapon.h"

/// \brief harpoon weapon; the rope simulation itself lives in PlayerHarpoon, this is the inventory side.
class Harpoon : public Weapon
{
public:
   /// \brief constructs the harpoon and tags it with its weapon type.
   Harpoon();

   /// \brief returns the weapon name used by gameplay and config code.
   /// \return string literal "harpoon".
   std::string getName() const override;
};
