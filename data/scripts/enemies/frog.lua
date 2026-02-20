require "data/scripts/enemies/constants"
v2d = require "data/scripts/enemies/vectorial2"

-- frog = 2 x 2 x 24
--
-- +---+---+
-- | xx|xx |
-- +---+---+
-- | xx|xx |
-- +---+---+
-- 0 .... 48

--  7px offset to bottom
-- 10px offset to top
--  3px offset to sides

-- sprite rows:
-- 1) idle left (8 frames)
-- 2) idle blink left (8 frames)
-- 3) attack left ( 6 frames)
-- 4) idle right (8 frames)
-- 5) idle blink right (8 frames)
-- 6) attack right (6 frames)
-- 7-9) unused
-- 10) dying animation (24 frames) - both left and right aligned
--
-- his tongue can be extended by using the sprites next to rows 3 and 6

-- tongue left:
--    tongue extension: 288, 120
--    tongue tip 312, 120 (24x24)
-- tongue right:
--    tongue extension: 288, 264
--    tongue tip: 312, 264 (24x24)

-- tongue animation logic
-- 1. the attack animation plays through all frames at regular speed
-- 2. the tongue only starts extending after the attack animation has completed
-- 3. the mouth stays on the last frame (open) while the tongue is extending and retracting
-- 4. when the tongue is fully retracted, the mouth animation plays backwards from the last frame to frame 0 (closed)
-- 5. once the backward animation reaches frame 0, it stays there until the state changes back to idle

ROW_OFFSET_RIGHT = 3
FRAME_COUNTS = {8, 8, 6, 19}
DEBUG_ATTACK_OFFSET = 0 * 48  -- 8x48 px offset when attacking

MIN_ATTACK_WAIT = 1.5  -- Minimum wait time in seconds between attacks

MAX_TONGUE_SCALE = 5.0  -- Maximum scale factor for the tongue extension
SPRITE_WIDTH = 48
SPRITE_HEIGHT = 48

SPRITE_WIDTH_DYING = 72
SPRITE_HEIGHT_DYING = 72
SPRITE_ROW_DYING = 6

STATE_IDLE = 1
STATE_ATTACK = 2
STATE_DYING = 3

CYCLE_IDLE = 1
CYCLE_BLINK = 2
CYCLE_ATTACK = 3
CYCLE_DYING = 4


------------------------------------------------------------------------------------------------------------------------
-- member variables
_done = false
_position = v2d.Vector2D(0, 0)
_player_position = v2d.Vector2D(0, 0)
_elapsed = 0
_alignment_offset = 0
_state = STATE_IDLE
_points_left = true
_tongue_direction_multiplier = -1  -- -1 for left, 1 for right
_smashed = false
_animation_frame = 0
_prev_state = STATE_IDLE
_is_blinking = false

-- animation speed factors for different states
_idle_anim_speed = 1.0
_attack_anim_speed = 2.0
_blink_anim_speed = 1.0

-- attack state variables
_attack_delay = 1.0  -- time in seconds the frog cannot attack after attacking
_attack_cooldown = 0  -- current cooldown time remaining (may not be needed with proper retraction)
_tongue_scale = 0.0  -- Current tongue extension scale (0.0 to 5.0)
_tongue_fully_extended = false  -- Flag to track if tongue is fully extended
_tongue_hit_player = false        -- Flag to track if tongue has hit the player
_retracting_tongue = false        -- Flag to track if tongue is retracting
_attack_animation_time = 0.0      -- Time elapsed in the current attack animation phase
_attack_animation_time_at_full_retraction = nil  -- Time when tongue became fully retracted
_backward_animation_completed = false  -- Flag to track if backward animation has completed
_attack_animation_speed = 10.0
_tongue_speed = 10.0
_last_attack_time = 0.0  -- Time of the last attack

-- health and death variables
_energy = 30  -- frog's health points
_dead = false  -- whether the frog is dead
_key_pressed = 0  -- track pressed keys for movement

-- dying animation variables
_death_animation_frame = 0
_death_animation_finished = false

-- sprite row tracking for debug
_prev_log_sprite_row = nil

-- animation tracking
_prev_animation_frame = 0

-- tongue sprite dimensions
_tongue_extension_x = 288
_tongue_extension_y = 120
_tongue_tip_x = 312
_tongue_tip_y = 120


------------------------------------------------------------------------------------------------------------------------
properties = {
   sprite = "data/sprites/enemy_frog.png",
   damage = 1,
   smash = true,
   velocity_walk_max = 0.0,  -- frogs don't walk, so set to 0
   acceleration_ground = 0.0  -- frogs don't accelerate, so set to 0
}


------------------------------------------------------------------------------------------------------------------------
function initialize()
   addShapeRect(0.4, 0.25, 0.0, 0.05)
   -- setSpriteOffset(0, 0, 24);

   -- add hitbox for damage detection
   addHitbox(-18, -18, 36, 36)  -- x offset, y offset, width, height

   -- initialize sprite slot for stretched tongue (hidden by default)
   addSprite()
   setSpriteOffset(1, 36, 12)
   setSpriteScale(1, 0, 0)

   -- tip of the tongue
   addSprite()
   setSpriteOffset(2, 36, 12)
   setSpriteVisible(2, false)

   updateSprite(0.0)
   print("frog.lua initialized")
end


------------------------------------------------------------------------------------------------------------------------
function checkAttackCondition(next_state)

   -- only check for attack transition if we're in a state that allows it
   if next_state == _state and _state ~= STATE_DYING and _state ~= STATE_ATTACK then
      -- check for attack transition
      x_diff = _player_position:getX() // 24 - _position:getX() // 24
      y_diff = _player_position:getY() // 24 - _position:getY() // 24

      x_in_range =
         (_points_left and x_diff >= -5 and x_diff <= 0) or
         (not _points_left and x_diff <= 5 and x_diff >= 0)

      y_in_range = y_diff >= -1 and y_diff <= 1

      local tongue_retracted = _tongue_scale <= 0.01

      -- check minimum wait time between attacks
      local time_since_last_attack = _elapsed - _last_attack_time
      local can_attack = time_since_last_attack >= MIN_ATTACK_WAIT

      if (y_in_range and x_in_range and tongue_retracted and can_attack) then
         if (
            isPhsyicsPathClear(
               _position:getX(),
               _position:getY(),
               _player_position:getX(),
               _player_position:getY()
            )
         ) then
            print("Frog: Attack triggered, x_diff: " .. x_diff)
            next_state = STATE_ATTACK
         end
      end
   end

   return next_state
end


------------------------------------------------------------------------------------------------------------------------
function resetOnStateTransition()
   _animation_frame = 0
   _tongue_scale = 0.0
   _tongue_fully_extended = false
   _tongue_hit_player = false
   _retracting_tongue = false
   _attack_animation_time = 0.0
   _attack_animation_time_at_full_retraction = nil
   _backward_animation_completed = false
end

------------------------------------------------------------------------------------------------------------------------
function updateState(dt)

   local next_state
    
   if _state == STATE_DYING then
      next_state = updateStateDying(dt)
   elseif _state == STATE_ATTACK then
      next_state = updateStateAttack(dt)
   else
      next_state = updateStateIdle(dt)
   end

   next_state = checkAttackCondition(next_state)

   if (next_state ~= _state) then

      _prev_state = _state
      _state = next_state

      local state_names = {[STATE_IDLE] = "IDLE", [STATE_ATTACK] = "ATTACK", [STATE_DYING] = "DYING"}
      print("Frog: State changed from " .. state_names[_prev_state] .. " to " .. state_names[_state])

      resetOnStateTransition()

      if next_state == STATE_ATTACK then
         _last_attack_time = _elapsed
      end
   end

   if _death_animation_finished then
      die()
   end
end

------------------------------------------------------------------------------------------------------------------------
function logSprite(dt)

   local current_row = math.floor(y / SPRITE_HEIGHT)

   if _prev_log_sprite_row ~= current_row then

      _prev_log_sprite_row = current_row

      local state_names = {[STATE_IDLE] = "IDLE", [STATE_ATTACK] = "ATTACK", [STATE_DYING] = "DYING"}
      local cycle_names = {[CYCLE_IDLE] = "IDLE", [CYCLE_BLINK] = "BLINK", [CYCLE_ATTACK] = "ATTACK"}

      if _state == STATE_DYING then
         print("Frog: Using death animation row " .. current_row .. " for state " .. state_names[_state])
      elseif _state == STATE_ATTACK then
         print("Frog: Using attack animation row " .. current_row .. " for state " .. state_names[_state])
      else  -- IDLE state
         local cycle = _is_blinking and CYCLE_BLINK or CYCLE_IDLE
         print("Frog: Using " .. cycle_names[cycle] .. " animation row " .. current_row .. " for state " .. state_names[_state])
      end

   end
end

------------------------------------------------------------------------------------------------------------------------
function updateSprite(dt)

   local x = 0
   local y = 0
   local width = 0
   local height = 0

   if _state == STATE_DYING then
      x, y, width, height = getDyingSpriteCoords()
   elseif _state == STATE_ATTACK then
      x, y, width, height = getAttackSpriteCoords()
   elseif _state == STATE_IDLE then
      x, y, width, height = getIdleSpriteCoords()
   end

   updateSpriteRect(
      0,
      x,
      y,
      width,
      height
   )

   if _state == STATE_ATTACK then
      updateSpriteAttack(dt)
   end
end


------------------------------------------------------------------------------------------------------------------------
-- function to handle attack state updates
function updateSpriteAttack(dt)
   local current_attack_frame = math.floor(_animation_frame)

   -- calculate distance to player to determine tongue length
   local dist_to_player = math.abs(_player_position:getX() - _position:getX())

   -- Calculate the current frame index to determine if animation has completed
   local current_frame_index = math.floor(_attack_animation_time * _attack_animation_speed * _attack_anim_speed)
   local max_frames = FRAME_COUNTS[CYCLE_ATTACK] - 1  -- Max frames for attack animation
   local animation_completed = current_frame_index >= max_frames

   -- extend or retract
   if not _retracting_tongue then
      -- Only start extending the tongue after the attack animation has completed
      if animation_completed then
         _tongue_scale = _tongue_scale + dt * _tongue_speed
         if _tongue_scale >= MAX_TONGUE_SCALE and not _retracting_tongue then
            _tongue_scale = MAX_TONGUE_SCALE
            _tongue_fully_extended = true
            _retracting_tongue = true
         end
      end
   else
      -- retract
      _tongue_scale = _tongue_scale - dt * _tongue_speed
      if _tongue_scale <= 0.0 then
         _tongue_scale = 0.0
         -- Record the time when tongue became fully retracted (if not already recorded)
         if not _attack_animation_time_at_full_retraction then
            _attack_animation_time_at_full_retraction = _attack_animation_time
         end
      end
   end

   setSpriteScale(1, _tongue_scale, 1.0)
   setSpriteVisible(2, _tongue_scale >= 0.01)

   -- print(" " .. tostring(_tongue_scale <= 0.01))

   -- calculate the actual displayed tongue width based on scale
   local displayed_tongue_width = 24 * _tongue_scale

   -- check for collision between tongue and player using the engine's helper function
   local tongue_x, tongue_y, tongue_w, tongue_h

   local base_x, base_y
   base_x = _position:getX()
   base_y = _position:getY()

   if _points_left then
      tongue_x = base_x - displayed_tongue_width
   else
      tongue_x = base_x
   end

   -- use the engine's intersectsWithPlayer function to check for collision
   if intersectsWithPlayer(tongue_x, base_y, displayed_tongue_width, 24) then
       damage(properties.damage, 0, 0)
       _tongue_hit_player = true
       _retracting_tongue = true
   end

   local tip_offset_x = _tongue_direction_multiplier * (24 * _tongue_scale) + 16 * _tongue_direction_multiplier  -- Use multiplier for direction and add/subtract 1

   setSpriteOffset(2, tip_offset_x, 12)
end

------------------------------------------------------------------------------------------------------------------------
function smashed()

   if (_smashed or _state == STATE_DYING) then
      return
   end

   print("Frog: Smashed, starting to die")

   _smashed = true
   startDying()
end


------------------------------------------------------------------------------------------------------------------------
function startDying()
   _state = STATE_DYING
   _death_animation_frame = 0
   _elapsed = 0.0
   setSpriteOffset(1, 36, 0)
   setSpriteVisible(1, false) -- hide the tongue
   setSpriteVisible(2, false) -- hide the tongue

   -- when dying, stop causing damage to the player
   setDamage(0)
end


------------------------------------------------------------------------------------------------------------------------
function hit(damage_value)

   if (_dead or _state == STATE_DYING) then
      return
   end

   _energy = _energy - damage_value
   print("Frog: Hit for " .. damage_value .. " damage, remaining energy: " .. _energy)

   if (_energy <= 0) then
      print("Frog: Energy depleted, starting to die")
      startDying()
   end
end


------------------------------------------------------------------------------------------------------------------------
function timeout(id)
end


------------------------------------------------------------------------------------------------------------------------
function retrieveProperties()
   updateProperties(properties)
end


------------------------------------------------------------------------------------------------------------------------
function movedTo(x, y)
   _position = v2d.Vector2D(x, y)
end


------------------------------------------------------------------------------------------------------------------------
function playerMovedTo(x, y)

   if (_done) then
      return
   end

   _player_position = v2d.Vector2D(x, y)
   distance_to_player = (_player_position - _position):getLength()
end


------------------------------------------------------------------------------------------------------------------------
function update(dt)

   _elapsed = _elapsed + dt

   updateState(dt)
   updateSprite(dt)
   updateKeysPressed(_key_pressed)
end


------------------------------------------------------------------------------------------------------------------------
function setPath(name, table)
end


------------------------------------------------------------------------------------------------------------------------
function keyPressed(key)
   _key_pressed = (_key_pressed | key)
end


------------------------------------------------------------------------------------------------------------------------
function keyReleased(key)
   _key_pressed = _key_pressed & (~key)
end


------------------------------------------------------------------------------------------------------------------------
function writeProperty(key, value)

   if (key == "alignment") then

      _tongue_tip_x = 312
      _tongue_extension_x = 288

      if (value == "right") then
         _alignment_offset = 3 * SPRITE_HEIGHT
         _points_left = false
         _tongue_direction_multiplier = 1  -- 1 for right
         _tongue_extension_y = 264
         _tongue_tip_y = 264
      else
         _tongue_direction_multiplier = -1  -- -1 for left (default)
         _tongue_extension_y = 120
         _tongue_tip_y = 120
      end

      updateSpriteRect(
         1,
         _tongue_extension_x,
         _tongue_extension_y,
         24,
         24
      )

      updateSpriteRect(
         2,
         _tongue_tip_x,
         _tongue_tip_y,
         24,
         24
      )

   elseif (key == "idle_anim_speed") then
      _idle_anim_speed = value
   elseif (key == "attack_anim_speed") then
      _attack_anim_speed = value
   elseif (key == "blink_anim_speed") then
      _blink_anim_speed = value
   elseif (key == "attack_delay") then
      _attack_delay = value
   end
end


------------------------------------------------------------------------------------------------------------------------
-- Functions to get sprite coordinates for each state
function getIdleSpriteCoords()
   local cycle = CYCLE_IDLE
   local max_frames = FRAME_COUNTS[cycle]
   
   -- determine the animation cycle based on state and blinking
   if _is_blinking then
      cycle = CYCLE_BLINK
      max_frames = FRAME_COUNTS[cycle]
   end
   
   local frame_index = math.floor(_animation_frame) % max_frames
   local x = frame_index * SPRITE_WIDTH
   local y = (cycle - 1) * SPRITE_HEIGHT
   y = y + _alignment_offset
   
   return x, y, SPRITE_WIDTH, SPRITE_WIDTH
end

------------------------------------------------------------------------------------------------------------------------
function getAttackSpriteCoords()
   local cycle = CYCLE_ATTACK
   local max_frames = FRAME_COUNTS[cycle] - 1

   -- Calculate the frame index based on animation time and speed
   local frame_index = math.floor(_attack_animation_time * _attack_animation_speed * _attack_anim_speed)

   -- Determine the current animation frame based on the tongue state
   if _retracting_tongue and _tongue_scale <= 0.0 and not _backward_animation_completed then

      -- When fully retracted after being in retraction, animate backwards from last frame to frame 0
      -- Calculate how much time has passed since tongue scale became <= 0
      local time_since_full_retraction = math.max(0, _attack_animation_time - (_attack_animation_time_at_full_retraction or _attack_animation_time))

      -- Calculate frame index going backwards from max to 0 based on time since full retraction
      local frames_since_full_retraction = math.floor(time_since_full_retraction * _attack_animation_speed * _attack_anim_speed)
      frame_index = math.max(0, max_frames - frames_since_full_retraction)
      
      -- Check if backward animation has completed (reached frame 0)
      if frame_index == 0 then
         _backward_animation_completed = true
      end
   elseif _retracting_tongue and _tongue_scale <= 0.0 and _backward_animation_completed then
      -- When backward animation is completed, stay at frame 0
      frame_index = 0
   elseif _retracting_tongue or _tongue_fully_extended then
      -- When extending, retracting, or fully extended, stay on the last frame
      frame_index = max_frames
   else
      -- Going forward from 0 to max at regular speed during initial animation, then stay at max
      frame_index = math.min(frame_index, max_frames)
   end

   -- calculate sprite coordinates
   local x = frame_index * SPRITE_WIDTH
   local y = (cycle - 1) * SPRITE_HEIGHT

   -- Apply attack sprite offset when attacking
   x = x + DEBUG_ATTACK_OFFSET

   -- Apply alignment offset for non-dying states
   y = y + _alignment_offset

   return x, y, SPRITE_WIDTH, SPRITE_HEIGHT
end

------------------------------------------------------------------------------------------------------------------------
function getDyingSpriteCoords()
   local death_frame_index = math.floor(_death_animation_frame)
   
   local x = death_frame_index * SPRITE_WIDTH_DYING
   local y = SPRITE_ROW_DYING * SPRITE_HEIGHT_DYING
   return x, y, SPRITE_WIDTH_DYING, SPRITE_HEIGHT_DYING
end

------------------------------------------------------------------------------------------------------------------------
-- function to update dying state
function updateStateDying(dt)
   _death_animation_frame = _death_animation_frame + dt * 10.0
   local death_frame_index = math.floor(_death_animation_frame)

   -- check if death animation is finished
   if death_frame_index >= FRAME_COUNTS[CYCLE_DYING] then
      if not _death_animation_finished then
         _death_animation_finished = true
         _dead = true
         print("Frog: Death animation finished, frog is now dead")
      end
   end
   
   return STATE_DYING
end

------------------------------------------------------------------------------------------------------------------------
-- function to update attack state
function updateStateAttack(dt)
   _attack_animation_time = _attack_animation_time + dt

   -- return to idle only when tongue is fully retracted AND backward animation is completed
   if _tongue_scale <= 0.0 and _retracting_tongue and _backward_animation_completed then
      return STATE_IDLE
   else
      return STATE_ATTACK
   end
end

------------------------------------------------------------------------------------------------------------------------
-- function to update idle state
function updateStateIdle(dt)
   local anim_speed_factor = _idle_anim_speed    -- idle animation speed
   _animation_frame = _animation_frame + dt * 10.0 * anim_speed_factor  -- 10 fps base rate

   -- Check if an animation cycle has completed
   local current_frame_idx = math.floor(_animation_frame) % FRAME_COUNTS[CYCLE_IDLE]
   local prev_frame_idx = math.floor(_prev_animation_frame) % FRAME_COUNTS[CYCLE_IDLE]

   -- If we've gone from a higher frame number to a lower one, a cycle has completed
   if prev_frame_idx > current_frame_idx and prev_frame_idx >= FRAME_COUNTS[CYCLE_IDLE] - 1 then
      _is_blinking = math.random(1, 20) == 1
   end

   _prev_animation_frame = _animation_frame
   
   return STATE_IDLE
end


