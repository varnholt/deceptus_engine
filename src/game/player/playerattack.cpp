#include "playerattack.h"
#include "framework/tools/stopwatch.h"
#include "game/audio/audio.h"
#include "game/constants.h"
#include "game/player/playeranimation.h"
#include "game/player/playercontrols.h"
#include "game/player/weaponsystem.h"
#include "game/savestate.h"
#include "game/weapons/bow.h"
#include "game/weapons/gun.h"
#include "game/weapons/sword.h"

#if defined __GNUC__ && __linux__
#define FMT_HEADER_ONLY
#include <fmt/core.h>
#include <ctime>
#else
namespace fmt = std;
#endif

void PlayerAttack::attack(
   const std::shared_ptr<b2World>& world,
   const std::shared_ptr<PlayerControls>& controls,
   const std::shared_ptr<PlayerAnimation>& animation,
   const sf::Vector2f& player_pos_px,
   bool points_to_left,
   bool in_air
)
{
   const auto& weapon_system = SaveState::getPlayerInfo()._weapons;

   if (!weapon_system._selected)
   {
      return;
   }

   b2Vec2 pos;
   b2Vec2 dir;
   dir.x = points_to_left ? -1.0f : 1.0f;
   dir.y = 0.0f;

   switch (weapon_system._selected->getWeaponType())
   {
      case WeaponType::Bow:
      {
         dir.y = -0.1f;

         constexpr auto force = 1.5f;
         const auto x_offset = dir.x * 0.5f;
         const auto y_offset = -0.25f;

         pos.x = x_offset + player_pos_px.x * MPP;
         pos.y = y_offset + player_pos_px.y * MPP;

         if (_attack_button_pressed && !_attack_button_was_pressed)
         {
            _timepoint_attack_start = StopWatch::getInstance().now();
         }

         dynamic_pointer_cast<Bow>(weapon_system._selected)->useInIntervals(world, pos, force * dir);
         break;
      }
      case WeaponType::Gun:
      {
         constexpr auto force = 10.0f;
         const auto x_offset = dir.x * 1.0f;
         const auto y_offset = -0.1f;

         pos.x = x_offset + player_pos_px.x * MPP;
         pos.y = y_offset + player_pos_px.y * MPP;

         if (_attack_button_pressed && !_attack_button_was_pressed)
         {
            _timepoint_attack_start = StopWatch::getInstance().now();
         }

         dynamic_pointer_cast<Gun>(weapon_system._selected)->useInIntervals(world, pos, force * dir);
         break;
      }
      case WeaponType::Sword:
      {
         // no 2nd strike without new button press
         if (_attack_button_pressed && _attack_button_was_pressed)
         {
            return;
         }

         // no 2nd strike when previous animation is not elapsed
         const auto attack_duration = animation->getActiveAttackCycleDuration();
         if (attack_duration.has_value())
         {
            const auto duration_since_attack = StopWatch::getInstance().now() - _timepoint_attack_start;
            const auto attack_elapsed = (duration_since_attack > attack_duration.value());

            if (!attack_elapsed)
            {
               return;
            }
         }

         // for the sword weapon we also have to store the times when the player attacks while
         // bending down, in-air or while standing; they need to be distinguished so the player animation
         // knows which animation to play (even if bend down or jump is no longer pressed)
         const auto now = StopWatch::getInstance().now();
         _timepoint_attack_start = now;

         if (in_air)
         {
            _timepoint_attack_jumping_start = now;
            Audio::getInstance().playSample({fmt::format("player_sword_standing_{:02}.wav", (std::rand() % 9) + 1)});
         }
         else if (controls->isBendDownActive())
         {
            _timepoint_attack_bend_down_start = now;
            Audio::getInstance().playSample({fmt::format("player_sword_kneeling_{:02}.wav", (std::rand() % 4) + 1)});
         }
         else
         {
            _timepoint_attack_standing_start = now;
            controls->lockOrientation(std::chrono::duration_cast<std::chrono::milliseconds>(animation->getSwordAttackDurationStanding()));
            Audio::getInstance().playSample({fmt::format("player_sword_standing_{:02}.wav", (std::rand() % 9) + 1)});
         }

         dynamic_pointer_cast<Sword>(weapon_system._selected)->use(world, dir);
         break;
      }
      case WeaponType::None:
      {
         break;
      }
   }
}

bool PlayerAttack::isAttacking() const
{
   return _attack_button_pressed;
}
