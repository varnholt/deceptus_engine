#ifndef PLAYERATTACKDASH_H
#define PLAYERATTACKDASH_H

#include <SFML/System.hpp>
#include <cstdint>

#include "game/constants.h"

class b2Body;

class PlayerAttackDash
{
public:
   struct DashInput
   {
      Dash _dir;
      b2Body* player_body{nullptr};
   };

   PlayerAttackDash() = default;

   void update(const sf::Time& /*dt*/);
   void reset(const DashInput& input);

private:
   DashInput _input;
   int32_t _frame{0};
   int32_t _frame_count{0};
   float _attack_multiplier{0};
};

#endif  // PLAYERATTACKDASH_H
