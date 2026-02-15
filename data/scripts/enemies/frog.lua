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

CYCLE_IDLE = 1
CYCLE_BLINK = 2
CYCLE_ATTACK = 3
ROW_OFFSET_RIGHT = 3
FRAME_COUNTS = {8, 8, 6, 24}  -- Added 24 frames for dying animation
DEATH_ROW = 9  -- Row index for death animation (counting from 0)
DEBUG_ATTACK_OFFSET = 0 * 48  -- 8x48 px offset when attacking
TONGUE_PART_1 = {288, 96} -- height: 48px, width: 24px
TONGUE_PART_2 = {312, 96} -- height: 48px, width: 24px

MAX_TONGUE_SCALE = 5.0  -- Maximum scale factor for the tongue extension
SPRITE_WIDTH = 48
SPRITE_HEIGHT = 48

STATE_IDLE = 1
STATE_ATTACK = 2
STATE_DYING = 3


------------------------------------------------------------------------------------------------------------------------
-- member variables
_done = false
_position = v2d.Vector2D(0, 0)
_player_position = v2d.Vector2D(0, 0)
_elapsed = 0
_alignment_offset = 0
_state = STATE_IDLE
_points_left = true
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

-- health and death variables
_energy = 30  -- frog's health points
_dead = false  -- whether the frog is dead
_key_pressed = 0  -- track pressed keys for movement

-- dying animation variables
_death_animation_frame = 0
_death_animation_finished = false

-- sprite row tracking for debug
_prev_sprite_row = nil

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
   damage = 4,
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
   updateSpriteRect(
      1,           -- sprite slot for tongue
      240,         -- x in sprite sheet for tongue
      80,          -- y in sprite sheet for tongue
      24,          -- width in sprite sheet
      16           -- height in sprite sheet
   )

   -- hide tongue initially
   setSpriteScale(1, 0, 0)

   updateSprite(0.0)
   print("frog.lua initialized")
end


function checkAttackCondition(next_state)

   -- only check for attack transition if we're in a state that allows it
   -- (not in attack or dying state)
   if next_state == _state and _state ~= STATE_DYING and _state ~= STATE_ATTACK then
      -- check for attack transition
      x_diff = _player_position:getX() // 24 - _position:getX() // 24
      y_diff = _player_position:getY() // 24 - _position:getY() // 24

      x_in_range =
         (_points_left and x_diff >= -5 and x_diff <= 0) or
         (not _points_left and x_diff <= 5 and x_diff >= 0)

      y_in_range = y_diff >= -1 and y_diff <= 1

      -- Check if the tongue is fully retracted (ready for next attack)
      local tongue_retracted = _tongue_scale <= 0.01  -- using small epsilon instead of exact zero

      if (y_in_range and x_in_range and tongue_retracted) then
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
function updateState(dt)

   local next_state
    
   if _state == STATE_DYING then
      updateStateDying(dt)
      next_state = STATE_DYING
   elseif _state == STATE_ATTACK then
      next_state = updateStateAttack(dt)
   elseif _is_blinking then
      next_state = updateStateBlinking(dt)
   else
      next_state = updateStateIdle(dt)
   end

   next_state = checkAttackCondition(next_state)

   if (next_state ~= _state) then

      _prev_state = _state
      _state = next_state

      local state_names = {[STATE_IDLE] = "IDLE", [STATE_ATTACK] = "ATTACK", [STATE_DYING] = "DYING"}
      print("Frog: State changed from " .. state_names[_prev_state] .. " to " .. state_names[_state])

      -- reset all on each state transition
      _animation_frame = 0
      _tongue_scale = 0.0
      _tongue_fully_extended = false
      _tongue_hit_player = false
      _retracting_tongue = false
      _attack_animation_time = 0.0
   end

   if _death_animation_finished then
      die()
   end
end

------------------------------------------------------------------------------------------------------------------------
function logSprite(dt)

   local current_row = math.floor(y / SPRITE_HEIGHT)

   if _prev_sprite_row ~= current_row then

      _prev_sprite_row = current_row

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

   if _state == STATE_DYING then
      x, y = getDyingSpriteCoords()
   elseif _state == STATE_ATTACK then
      x, y = getAttackSpriteCoords()
   elseif _state == STATE_IDLE then
      x, y = getIdleSpriteCoords()
   end

   updateSpriteRect(
      0,
      x,
      y,
      SPRITE_WIDTH,
      SPRITE_HEIGHT
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

   -- extend or retract
   if not _retracting_tongue then
      -- extending
      _tongue_scale = _tongue_scale + dt * 1
      if _tongue_scale >= MAX_TONGUE_SCALE and not _retracting_tongue then
         _tongue_scale = MAX_TONGUE_SCALE
         _tongue_fully_extended = true
         _retracting_tongue = true
      end
   else
      -- retract
      _tongue_scale = _tongue_scale - dt * 1
      if _tongue_scale <= 0.0 then
         _tongue_scale = 0.0
      end
   end

   setSpriteScale(1, _tongue_scale, 1.0)

--   -- debug output for tongue scale and frame
--   if _tongue_scale > 0 then
--      print("Frog tongue scale: " .. _tongue_scale .. ", frame: " .. current_attack_frame)
--   end

   -- calculate the actual displayed tongue width based on scale
   local displayed_tongue_width = 24 * _tongue_scale

   -- check for collision between tongue and player using the engine's helper function
   local tongue_x, tongue_y, tongue_w, tongue_h

   setSpriteOffset(1, 0, 0)

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
end

------------------------------------------------------------------------------------------------------------------------
function smashed()

   if (_smashed or _state == STATE_DYING) then
      return
   end

   _smashed = true
   print("Frog: Smashed, starting to die")
   startDying()
end


------------------------------------------------------------------------------------------------------------------------
function startDying()
   _state = STATE_DYING
   _death_animation_frame = 0
   _elapsed = 0.0

   print("Frog: Starting to die")

--   -- make sure the thing stops to move
--   if _key_pressed then
--      keyReleased(Key["KeyLeft"])
--      keyReleased(Key["KeyRight"])
--   end

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
      if (value == "right") then
         _alignment_offset = 3 * SPRITE_HEIGHT
         _points_left = false
         _tongue_extension_y = 264
         _tongue_tip_y = 264
      else
         _tongue_extension_y = 120
         _tongue_tip_y = 120
      end
      _tongue_tip_x = 312
      _tongue_extension_x = 288
      updateSpriteRect(
         1,
         _tongue_extension_x,
         _tongue_extension_y,
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
   
   return x, y
end

------------------------------------------------------------------------------------------------------------------------
function getAttackSpriteCoords()
   local cycle = CYCLE_ATTACK
   local max_frames = FRAME_COUNTS[cycle] - 1  -- Adjust to 0-indexed

   -- Calculate the frame index based on animation time and speed
   local anim_speed_factor = _attack_anim_speed
   local frame_rate = 10.0  -- Base frame rate
   local frame_index = math.floor(_attack_animation_time * frame_rate * anim_speed_factor)

   -- Determine the current animation frame based on the tongue state
   if _retracting_tongue and _tongue_scale <= 0.0 then
      -- When fully retracted after being in retraction, go back to frame 0
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

   return x, y
end

------------------------------------------------------------------------------------------------------------------------
function getDyingSpriteCoords()
   local death_frame_index = math.floor(_death_animation_frame)
   
   local x = death_frame_index * SPRITE_WIDTH
   local y = DEATH_ROW * SPRITE_HEIGHT
   
   y = y + _alignment_offset
   
   return x, y
end

------------------------------------------------------------------------------------------------------------------------
-- function to update blinking logic when an idle animation cycle completes
function updateBlinkingOnCycleComplete()
   -- only handle blinking when in idle state and not already blinking
   if _state ~= STATE_IDLE or _is_blinking then
      return
   end

   -- note that during idle, the frog can also blink
   -- i'd just go by random and pick that animation cycle with a change from 1:20
   if math.random(1, 20) == 1 then
      _is_blinking = true
      _blink_timer = 0
      print("Frog: Blinking started")
   end
end

------------------------------------------------------------------------------------------------------------------------
-- function to update blinking state (when already blinking)
function updateBlinking()
   -- only handle blinking when in idle state and currently blinking
   if _state ~= STATE_IDLE or not _is_blinking then
      return
   end

   -- handle blinking state - finish when last frame is reached
   -- check if we've reached the last frame of the blink animation
   local max_blink_frames = FRAME_COUNTS[CYCLE_BLINK]
   if _animation_frame >= max_blink_frames then
      _is_blinking = false
      print("Frog: Blinking stopped")
   end
end

------------------------------------------------------------------------------------------------------------------------
-- function to update dying state
function updateStateDying(dt)
   _death_animation_frame = _death_animation_frame + dt * 10.0  -- 10 fps base rate for death animation
   local death_frame_index = math.floor(_death_animation_frame)

   -- check if death animation is finished
   if death_frame_index >= FRAME_COUNTS[4] then  -- FRAME_COUNTS[4] corresponds to death animation frames
      if not _death_animation_finished then
         _death_animation_finished = true
         _dead = true  -- mark as truly dead after animation finishes
         print("Frog: Death animation finished, frog is now dead")
      end
   end
   
   return STATE_DYING  -- stay in dying state
end

------------------------------------------------------------------------------------------------------------------------
-- function to update attack state
function updateStateAttack(dt)
   _attack_animation_time = _attack_animation_time + dt
   
   -- return to idle only when tongue is fully retracted (back to scale 0)
   if _tongue_scale <= 0.0 and _retracting_tongue then
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
      -- Call the blinking logic when an idle animation cycle completes
      updateBlinkingOnCycleComplete()
   end

   _prev_animation_frame = _animation_frame
   
   return STATE_IDLE
end

------------------------------------------------------------------------------------------------------------------------
-- function to update blink state
function updateStateBlinking(dt)
   local anim_speed_factor = _blink_anim_speed   -- blink animation speed
   _animation_frame = _animation_frame + dt * 10.0 * anim_speed_factor  -- 10 fps base rate
   updateBlinking()

   return STATE_IDLE  -- return to idle state when blinking is done
end
