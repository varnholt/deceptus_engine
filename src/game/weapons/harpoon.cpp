#include "harpoon.h"

Harpoon::Harpoon()
{
   _type = WeaponType::Harpoon;
}

std::string Harpoon::getName() const
{
   return "harpoon";
}
