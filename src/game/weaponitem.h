#ifndef WEAPONITEM_H
#define WEAPONITEM_H


struct WeaponItem
{
   enum class WeaponType
   {
      Slingshot,
      Pistol,
      Bazooka,
      Laser,
      Aliengun
   };


   WeaponItem();

   int damage() const;

   WeaponType mType;
   int mAmmo = -1;
};

#endif // WEAPONITEM_H
