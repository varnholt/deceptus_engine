#ifndef PLAYERATTACKDASH_H
#define PLAYERATTACKDASH_H

#include <SFML/System.hpp>
#include <cstdint>

#include "game/constants.h"

class b2Body;

/// \brief applies a short forward force burst after successful melee attacks.
class PlayerAttackDash
{
public:
   /// \brief input data needed to drive the attack dash impulse.
   struct DashInput
   {
      Dash _dir;
      b2Body* player_body{nullptr};
   };

   /// \brief constructs an idle attack-dash helper.
   PlayerAttackDash() = default;

   /// \brief advances the attack dash by one frame and applies horizontal force to the player body.
   void update(const sf::Time& /*dt*/);
   /// \brief starts a new attack dash sequence when a valid direction is provided.
   /// \param input dash direction and player body used for force application.
   void reset(const DashInput& input);

private:
   DashInput _input;
   int32_t _frame{0};
   int32_t _frame_count{0};
   float _attack_multiplier{0};
};

#endif  // PLAYERATTACKDASH_H
