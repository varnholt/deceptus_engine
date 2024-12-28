#include "playerattackdash.h"
#include <box2d/box2d.h>
#include "game/physics/physicsconfiguration.h"

void PlayerAttackDash::update(const sf::Time& /*dt*/)
{
   if (_frame_count == 0)
   {
      return;
   }

   _frame_count--;

   const auto& physics_config = PhysicsConfiguration::getInstance();

   _attack_multiplier -= physics_config._player_attack_dash_multiplier_decrement_per_frame;

   // that would indicate badly configured settings
   if (_attack_multiplier < 0.0f)
   {
      return;
   }

   const auto dash_vector = physics_config._player_attack_dash_multiplier_scale_per_frame * _attack_multiplier *
                            _input.player_body->GetMass() * physics_config._player_dash_vector;

   const auto impulse = (_input._dir == Dash::Left) ? -dash_vector : dash_vector;

   _input.player_body->ApplyForceToCenter(b2Vec2(impulse, 0.0f), false);
}

void PlayerAttackDash::reset(const DashInput& input)
{
   if (input._dir == Dash::None)
   {
      return;
   }

   _input = input;
   const auto& physics_config = PhysicsConfiguration::getInstance();
   _frame_count = physics_config._player_attack_dash_frame_count;
   _attack_multiplier = physics_config._player_attack_dash_multiplier;
}
