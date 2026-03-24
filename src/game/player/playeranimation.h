#pragma once

#include "box2d/box2d.h"
#include <SFML/Graphics.hpp>

#include <array>
#include <optional>
#include <unordered_map>

#include "game/animation/animation.h"
#include "game/animation/animationpool.h"
#include "game/constants.h"

/// \brief selects and advances active player animation cycles from movement, combat, and state data.
class PlayerAnimation
{
public:
   /// \brief constructs animation state with no active cycle until animations are loaded.
   PlayerAnimation() = default;
   /// \brief loads all player animation clips from the shared animation pool and rebuilds lookup tables.
   /// \param pool animation pool used to create named animation instances.
   void loadAnimations(AnimationPool& pool);

   using HighResDuration = std::chrono::high_resolution_clock::duration;
   using HighResTimePoint = std::chrono::high_resolution_clock::time_point;

   /// \brief frame snapshot consumed by the animation state selector.
   struct PlayerAnimationData
   {
      bool _dead = false;
      bool _in_air = false;
      bool _in_water = false;
      bool _hard_landing = false;
      bool _bending_down = false;
      bool _crouching = false;
      bool _points_left = false;
      bool _points_right = false;
      bool _climb_joint_present = false;
      bool _moving_left = false;
      bool _moving_right = false;
      bool _wall_sliding = false;
      bool _wall_jump_points_right = false;
      bool _jumping_through_one_way_wall = false;
      bool _attacking = false;
      DeathReason _death_reason = DeathReason::Invalid;
      WeaponType _weapon_type = WeaponType::None;
      std::optional<Dash> _dash_dir;
      b2Vec2 _linear_velocity = b2Vec2{0.0f, 0.0f};
      int32_t _jump_frame_count = 0;
      int32_t _dash_frame_count = 0;
      int32_t _death_count_current_level = 0;
      int32_t _checkpoint_index = 0;
      HighResTimePoint _timepoint_wallslide;
      HighResTimePoint _timepoint_walljump;
      HighResTimePoint _timepoint_doublejump;
      HighResTimePoint _timepoint_bend_down_start;
      HighResTimePoint _timepoint_bend_down_end;
      HighResTimePoint _timepoint_attack_start;
      HighResTimePoint _timepoint_attack_bend_down_start;
      HighResTimePoint _timepoint_attack_jumping_start;
      HighResTimePoint _timepoint_attack_standing_start;
   };

   /// \brief chooses the next animation cycle and advances current and auxiliary animations.
   /// \param dt elapsed frame time.
   /// \param data current movement, combat, and environment state for this frame.
   void update(const sf::Time& dt, const PlayerAnimationData& data);

   /// \brief gets the main animation cycle currently used for player rendering.
   /// \return shared pointer to the active primary animation.
   const std::shared_ptr<Animation>& getCurrentCycle() const;
   /// \brief gets the auxiliary overlay cycle, used for layered attack visuals.
   /// \return shared pointer to the active auxiliary animation, or null when inactive.
   const std::shared_ptr<Animation>& getAuxiliaryCycle() const;
   /// \brief gets the dedicated wall-slide dust animation effect.
   /// \return shared pointer to the wall-slide effect animation.
   const std::shared_ptr<Animation>& getWallslideAnimation() const;

   /// \brief gets the total duration of the current primary animation cycle.
   /// \return full duration of the active primary animation.
   HighResDuration getCurrentAnimationDuration() const;
   /// \brief gets total spawn reveal duration including the initial hidden delay.
   /// \return full reveal sequence duration.
   HighResDuration getRevealDuration() const;
   /// \brief gets the initial delay before reveal frames become visible.
   /// \return duration spent hidden at spawn before reveal frames play.
   HighResDuration getRevealStartDelay() const;
   /// \brief gets duration of the currently selected standing sword attack variant.
   /// \param points_left true to query the left-facing standing swing, false for right-facing.
   /// \return duration of the selected standing sword animation.
   HighResDuration getSwordAttackDurationStanding(bool points_left) const;
   /// \brief gets the duration of the currently active sword attack cycle when one is playing.
   /// \return attack duration for the active sword cycle, or empty when no tracked sword attack is active.
   std::optional<HighResDuration> getActiveAttackCycleDuration();
   /// \brief reports whether the current main cycle is one of the standing sword attack clips.
   /// \return true when a standing sword attack animation is currently active.
   bool isStandingSwordAttackPlayed() const;
   /// \brief preselects the next standing sword animation variant once the previous one has finished.
   void prepareNextSwordStandingAttack();

   /// \brief resets alpha on looped animations, used after dash motion-blur rendering.
   void resetAlpha();

private:
   enum class JumpReference : int32_t
   {
      Ignition = 0,
      Up = 1,
      Down = 2,
      Landing = 3
   };

   /// \brief maps an unarmed base animation to its armed equivalent for the active weapon.
   /// \param animation base animation candidate selected by movement logic.
   /// \param animation_data current animation context including equipped weapon type.
   /// \return mapped animation for armed state when available, otherwise the original animation.
   const std::shared_ptr<Animation>&
   getMappedArmedAnimation(const std::shared_ptr<Animation>& animation, const PlayerAnimationData& animation_data) const;

   /// \brief determines whether bend-up animation time is still active after leaving bend-down state.
   /// \param data current animation context.
   /// \return true when bend-up frames should still be displayed.
   bool isBendingUp(const PlayerAnimationData& data) const;

   /// \brief evaluates idle and idle-blink animation candidates.
   /// \param data current animation context.
   /// \return idle animation when idle conditions match, otherwise empty.
   std::optional<std::shared_ptr<Animation>> processIdleAnimation(const PlayerAnimationData& data);
   /// \brief evaluates bend-up animation candidate after crouch release.
   /// \param data current animation context.
   /// \return bend-up animation when bend-up timing is active, otherwise empty.
   std::optional<std::shared_ptr<Animation>> processBendUpAnimation(const PlayerAnimationData& data);
   /// \brief evaluates bend-down and bend-down-idle animation candidates.
   /// \param data current animation context.
   /// \return bend-down related animation when crouch-bend state is active, otherwise empty.
   std::optional<std::shared_ptr<Animation>> processBendDownAnimation(const PlayerAnimationData& data);
   /// \brief evaluates crouch-walk animation candidates.
   /// \param data current animation context.
   /// \return crouch-walk animation when supported and active, otherwise empty.
   std::optional<std::shared_ptr<Animation>> processCrouchAnimation(const PlayerAnimationData& data);
   /// \brief evaluates running animation candidates.
   /// \param data current animation context.
   /// \return run animation when grounded movement is active, otherwise empty.
   std::optional<std::shared_ptr<Animation>> processRunAnimation(const PlayerAnimationData& data);
   /// \brief evaluates dash init, dash loop, or dash stop animation candidates.
   /// \param data current animation context.
   /// \return dash animation matching dash frame phase, otherwise empty.
   std::optional<std::shared_ptr<Animation>> processDashAnimation(const PlayerAnimationData& data);
   /// \brief evaluates swimming animation candidates.
   /// \param data current animation context.
   /// \return swim animation when the player is in water, otherwise empty.
   std::optional<std::shared_ptr<Animation>> processSwimAnimation(const PlayerAnimationData& data);
   /// \brief evaluates wall-slide impact and loop animations.
   /// \param data current animation context.
   /// \return wall-slide animation when wall-sliding is active, otherwise empty.
   std::optional<std::shared_ptr<Animation>> processWallSlideAnimation(const PlayerAnimationData& data);
   /// \brief evaluates wall-jump animation candidate based on elapsed wall-jump timing.
   /// \param data current animation context.
   /// \return wall-jump animation while wall-jump timing is active, otherwise empty.
   std::optional<std::shared_ptr<Animation>> processWallJumpAnimation(const PlayerAnimationData& data);
   /// \brief evaluates double-jump animation candidate based on elapsed double-jump timing.
   /// \param data current animation context.
   /// \return double-jump animation while double-jump timing is active, otherwise empty.
   std::optional<std::shared_ptr<Animation>> processDoubleJumpAnimation(const PlayerAnimationData& data);
   /// \brief forces idle/swim animation during screen transition mode.
   /// \param data current animation context.
   /// \return transition-safe idle or swim animation when screen transition is active, otherwise empty.
   std::optional<std::shared_ptr<Animation>> processScreenTransitionIdleAnimation(const PlayerAnimationData& data);
   /// \brief evaluates jump phase animations from ignition through landing.
   /// \param data current animation context.
   /// \return jump-related animation when jump states match, otherwise empty.
   std::optional<std::shared_ptr<Animation>> processJumpAnimation(const PlayerAnimationData& data);
   /// \brief evaluates spawn reveal animation sequence.
   /// \param data current animation context.
   /// \return appear animation while reveal timing is active, otherwise empty.
   std::optional<std::shared_ptr<Animation>> processAppearAnimation(const PlayerAnimationData& data);
   /// \brief evaluates death animation candidate based on death reason and facing.
   /// \param data current animation context.
   /// \return death animation when player is dead, otherwise empty.
   std::optional<std::shared_ptr<Animation>> processDeathAnimation(const PlayerAnimationData& data);
   /// \brief evaluates weapon attack overlays and attack-cycle replacements.
   /// \param next_cycle movement-driven base cycle chosen before attack processing.
   /// \param data current animation context including weapon and attack timestamps.
   /// \return attack-specific cycle when attack visuals should override the base cycle, otherwise empty.
   std::optional<std::shared_ptr<Animation>>
   processAttackAnimation(const std::shared_ptr<Animation>& next_cycle, const PlayerAnimationData& data);

   std::shared_ptr<Animation> _idle_r;
   std::shared_ptr<Animation> _idle_l;

   std::shared_ptr<Animation> _idle_blink_r;
   std::shared_ptr<Animation> _idle_blink_l;
   std::shared_ptr<Animation> _sword_idle_blink_r;
   std::shared_ptr<Animation> _sword_idle_blink_l;

   std::shared_ptr<Animation> _sword_idle_l;
   std::shared_ptr<Animation> _sword_idle_r;
   std::shared_ptr<Animation> _idle_r_tmp;
   std::shared_ptr<Animation> _idle_l_tmp;

   std::shared_ptr<Animation> _bend_down_idle_l;
   std::shared_ptr<Animation> _bend_down_idle_r;
   std::shared_ptr<Animation> _bend_down_idle_blink_l;
   std::shared_ptr<Animation> _bend_down_idle_blink_r;
   std::shared_ptr<Animation> _sword_bend_down_idle_l;
   std::shared_ptr<Animation> _sword_bend_down_idle_r;
   std::shared_ptr<Animation> _sword_bend_down_idle_blink_l;
   std::shared_ptr<Animation> _sword_bend_down_idle_blink_r;
   std::shared_ptr<Animation> _bend_down_idle_l_tmp;
   std::shared_ptr<Animation> _bend_down_idle_r_tmp;

   std::shared_ptr<Animation> _bend_down_r;
   std::shared_ptr<Animation> _bend_down_l;
   std::shared_ptr<Animation> _sword_bend_down_l;
   std::shared_ptr<Animation> _sword_bend_down_r;
   std::shared_ptr<Animation> _bend_up_r;
   std::shared_ptr<Animation> _bend_up_l;
   std::shared_ptr<Animation> _sword_bend_up_l;
   std::shared_ptr<Animation> _sword_bend_up_r;

   std::shared_ptr<Animation> _idle_to_run_r;
   std::shared_ptr<Animation> _idle_to_run_l;
   std::shared_ptr<Animation> _runstop_r;
   std::shared_ptr<Animation> _runstop_l;
   std::shared_ptr<Animation> _run_r;
   std::shared_ptr<Animation> _run_l;
   std::shared_ptr<Animation> _sword_run_l;
   std::shared_ptr<Animation> _sword_run_r;

   std::shared_ptr<Animation> _dash_init_r;
   std::shared_ptr<Animation> _dash_init_l;
   std::shared_ptr<Animation> _dash_r;
   std::shared_ptr<Animation> _dash_l;
   std::shared_ptr<Animation> _dash_stop_r;
   std::shared_ptr<Animation> _dash_stop_l;
   std::shared_ptr<Animation> _sword_dash_init_r;
   std::shared_ptr<Animation> _sword_dash_init_l;
   std::shared_ptr<Animation> _sword_dash_r;
   std::shared_ptr<Animation> _sword_dash_l;
   std::shared_ptr<Animation> _sword_dash_stop_r;
   std::shared_ptr<Animation> _sword_dash_stop_l;

   std::shared_ptr<Animation> _crouch_r;
   std::shared_ptr<Animation> _crouch_l;

   std::shared_ptr<Animation> _jump_init_r;
   std::shared_ptr<Animation> _jump_up_r;
   std::shared_ptr<Animation> _jump_midair_r;
   std::shared_ptr<Animation> _jump_down_r;
   std::shared_ptr<Animation> _jump_landing_r;
   std::shared_ptr<Animation> _jump_init_l;
   std::shared_ptr<Animation> _jump_up_l;
   std::shared_ptr<Animation> _jump_midair_l;
   std::shared_ptr<Animation> _jump_down_l;
   std::shared_ptr<Animation> _jump_landing_l;

   std::shared_ptr<Animation> _sword_jump_init_r;
   std::shared_ptr<Animation> _sword_jump_up_r;
   std::shared_ptr<Animation> _sword_jump_midair_r;
   std::shared_ptr<Animation> _sword_jump_down_r;
   std::shared_ptr<Animation> _sword_jump_landing_r;
   std::shared_ptr<Animation> _sword_jump_init_l;
   std::shared_ptr<Animation> _sword_jump_up_l;
   std::shared_ptr<Animation> _sword_jump_midair_l;
   std::shared_ptr<Animation> _sword_jump_down_l;
   std::shared_ptr<Animation> _sword_jump_landing_l;

   // body for jump attack with sword
   std::shared_ptr<Animation> _sword_attack_jump_r;
   std::shared_ptr<Animation> _sword_attack_jump_l;

   // legs for jump attack with sword
   std::shared_ptr<Animation> _sword_attack_jump_legs_init_r;
   std::shared_ptr<Animation> _sword_attack_jump_legs_up_r;
   std::shared_ptr<Animation> _sword_attack_jump_legs_midair_r;
   std::shared_ptr<Animation> _sword_attack_jump_legs_down_r;
   std::shared_ptr<Animation> _sword_attack_jump_legs_landing_r;
   std::shared_ptr<Animation> _sword_attack_jump_legs_init_l;
   std::shared_ptr<Animation> _sword_attack_jump_legs_up_l;
   std::shared_ptr<Animation> _sword_attack_jump_legs_midair_l;
   std::shared_ptr<Animation> _sword_attack_jump_legs_down_l;
   std::shared_ptr<Animation> _sword_attack_jump_legs_landing_l;

   std::shared_ptr<Animation> _double_jump_r;
   std::shared_ptr<Animation> _double_jump_l;
   std::shared_ptr<Animation> _sword_double_jump_r;
   std::shared_ptr<Animation> _sword_double_jump_l;

   std::shared_ptr<Animation> _swim_r;
   std::shared_ptr<Animation> _swim_l;
   std::shared_ptr<Animation> _sword_swim_r;
   std::shared_ptr<Animation> _sword_swim_l;

   std::shared_ptr<Animation> _wallslide_impact_r;
   std::shared_ptr<Animation> _wallslide_impact_l;
   std::shared_ptr<Animation> _sword_wallslide_impact_r;
   std::shared_ptr<Animation> _sword_wallslide_impact_l;

   std::shared_ptr<Animation> _wallslide_r;
   std::shared_ptr<Animation> _wallslide_l;
   std::shared_ptr<Animation> _sword_wallslide_r;
   std::shared_ptr<Animation> _sword_wallslide_l;

   std::shared_ptr<Animation> _wall_jump_r;
   std::shared_ptr<Animation> _wall_jump_l;
   std::shared_ptr<Animation> _sword_wall_jump_r;
   std::shared_ptr<Animation> _sword_wall_jump_l;

   std::shared_ptr<Animation> _appear_r;
   std::shared_ptr<Animation> _appear_l;
   std::shared_ptr<Animation> _sword_appear_r;
   std::shared_ptr<Animation> _sword_appear_l;

   std::shared_ptr<Animation> _sword_attack_bend_down_1_l;
   std::shared_ptr<Animation> _sword_attack_bend_down_1_r;
   std::shared_ptr<Animation> _sword_attack_bend_down_2_l;
   std::shared_ptr<Animation> _sword_attack_bend_down_2_r;
   std::array<std::shared_ptr<Animation>, 2> _sword_attack_standing_l;
   std::array<std::shared_ptr<Animation>, 2> _sword_attack_standing_r;
   std::shared_ptr<Animation> _sword_attack_standing_tmp_l;
   std::shared_ptr<Animation> _sword_attack_standing_tmp_r;
   bool _sword_attack_standing_l_reset = false;
   bool _sword_attack_standing_r_reset = false;

   std::shared_ptr<Animation> _death_default;
   std::shared_ptr<Animation> _death_electrocuted_l;
   std::shared_ptr<Animation> _death_electrocuted_r;

   std::shared_ptr<Animation> _wallslide_animation;

   JumpReference _jump_animation_reference = JumpReference::Ignition;

   std::shared_ptr<Animation> _current_cycle;
   std::shared_ptr<Animation> _auxiliary_cycle;

   std::vector<std::shared_ptr<Animation>> _looped_animations;
   std::unordered_map<std::shared_ptr<Animation>, std::shared_ptr<Animation>> _sword_lut;
   std::unordered_map<std::shared_ptr<Animation>, std::shared_ptr<Animation>> _sword_attack_lut;
   std::vector<std::shared_ptr<Animation>> _appear_animations;
};
