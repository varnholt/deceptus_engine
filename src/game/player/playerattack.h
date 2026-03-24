#ifndef PLAYERATTACK_H
#define PLAYERATTACK_H

#include "box2d/box2d.h"
#include <SFML/Graphics.hpp>
#include <chrono>

class PlayerAnimation;
class PlayerControls;
class WeaponSystem;

/// \brief handles weapon attacks and keeps attack timing state for animation decisions.
struct PlayerAttack
{
   using HighResTimePoint = std::chrono::high_resolution_clock::time_point;

   bool _attack_button_was_pressed = false;
   bool _attack_button_pressed = false;

   HighResTimePoint _timepoint_attack_start;
   HighResTimePoint _timepoint_attack_bend_down_start;
   HighResTimePoint _timepoint_attack_standing_start;
   HighResTimePoint _timepoint_attack_jumping_start;

   enum class AttackResult
   {
      Executed,
      Discarded
   };

   /// \brief executes the currently equipped weapon attack when input and timing allow it.
   /// \param world box2d world instance.
   /// \param controls player controls used to query bend state and lock orientation for sword swings.
   /// \param animation player animation state used to gate follow-up sword attacks.
   /// \param player_pos_px player position in pixels, used as projectile spawn origin.
   /// \param points_to_left true when the player currently faces left.
   /// \param in_air true when the player is airborne.
   /// \return executed when an attack was fired or a sword swing was started, discarded otherwise.
   AttackResult attack(
      const std::shared_ptr<b2World>& world,
      const std::shared_ptr<PlayerControls>& controls,
      const std::shared_ptr<PlayerAnimation>& animation,
      const sf::Vector2f& player_pos_px,
      bool points_to_left,
      bool in_air
   );

   /// \brief reports whether the attack button is currently held down.
   /// \return true when attack input is active.
   bool isAttacking() const;
};

#endif  // PLAYERATTACK_H
