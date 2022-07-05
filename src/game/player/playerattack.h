#ifndef PLAYERATTACK_H
#define PLAYERATTACK_H

#include <chrono>

struct PlayerAttack
{
   using HighResTimePoint = std::chrono::high_resolution_clock::time_point;

   bool _was_attacking = false;
   bool _attacking = false;

   HighResTimePoint _timepoint_attack_start;

   bool isAttacking() const
   {
      return _attacking;
   }
};

#endif // PLAYERATTACK_H
