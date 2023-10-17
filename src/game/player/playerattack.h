#ifndef PLAYERATTACK_H
#define PLAYERATTACK_H

#include <chrono>

struct PlayerAttack
{
   using HighResTimePoint = std::chrono::high_resolution_clock::time_point;

   bool _attack_button_was_pressed = false;
   bool _attack_button_pressed = false;

   HighResTimePoint _timepoint_attack_start;
   HighResTimePoint _timepoint_attack_bend_down_start;
   HighResTimePoint _timepoint_attack_standing_start;
   HighResTimePoint _timepoint_attack_jumping_start;

   bool isAttacking() const
   {
      return _attack_button_pressed;
   }
};

#endif // PLAYERATTACK_H
