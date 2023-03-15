#ifndef WEAPONPROPERTIES_H
#define WEAPONPROPERTIES_H

#include <cstdint>
#include <memory>

class b2Body;
class b2Shape;

struct WeaponProperties
{
   b2Body* _parent_body{nullptr};
   mutable std::unique_ptr<b2Shape> _shape;

   // todo
   // the members below should rather be stored in a map holding variants
   int32_t _fire_interval_ms{0};
   int32_t _damage{0};
   float _gravity_scale = 0.0f;
};

#endif // WEAPONPROPERTIES_H
