#include "weaponitem.h"


WeaponItem::WeaponItem()
{

}


int WeaponItem::damage() const
{
   int val = 0;

   switch (mType)
   {
      case WeaponType::Slingshot:
      case WeaponType::Pistol:
      case WeaponType::Bazooka:
      case WeaponType::Laser:
      case WeaponType::Aliengun:
      default:
         val = 20;
         break;
   }

   return val;
}
