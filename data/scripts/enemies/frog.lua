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

CYCLE_IDLE = 1
CYCLE_BLINK = 2
CYCLE_ATTACK = 3
ROW_OFFSET_RIGHT = 3
FRAME_COUNTS = {8, 8, 6, 24}  -- Added 24 frames for dying animation
DEATH_ROW = 9  -- Row index for death animation (counting from 0)
DEBUG_ATTACK_OFFSET = 8 * 48  -- 8x48 px offset when attacking
TONGUE_PART_1 = {288, 96} -- height: 48px, width: 24px
TONGUE_PART_2 = {312, 96} -- height: 48px, width: 24px

SPRITE_WIDTH = 48
SPRITE_HEIGHT = 48

STATE_IDLE = 1
STATE_ATTACK = 2
STATE_DYING = 3


------------------------------------------------------------------------------------------------------------------------
-- Member variables
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
_attack_cooldown = 0  -- current cooldown time remaining
_tongue_scale = 0.0  -- Current tongue extension scale (0.0 to 5.0)

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
   setSpriteVisible(1, false)
   setSpriteScale(1, 0, 0)

   updateSprite(0.0)
   print("frog.lua initialized")
end



------------------------------------------------------------------------------------------------------------------------
function updateState(dt)

   -- update cooldown timer
   if _attack_cooldown > 0 then
      _attack_cooldown = _attack_cooldown - dt
      if _attack_cooldown < 0 then
         _attack_cooldown = 0
      end
   end

   local next_state
    
   -- Handle dying state separately
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

   -- Only check for attack transition if we're staying in the current state and it's not dying
   -- This ensures the attack animation completes before allowing another attack
   if next_state == _state and _state ~= STATE_DYING and _state ~= STATE_ATTACK then
      -- check for attack transition
      x_diff = _player_position:getX() // 24 - _position:getX() // 24
      y_diff = _player_position:getY() // 24 - _position:getY() // 24

      x_in_range =
         (_points_left and x_diff >= -5 and x_diff <= 0) or
         (not _points_left and x_diff <= 5 and x_diff >= 0)

      y_in_range = y_diff >= -1 and y_diff <= 1

      if (y_in_range and x_in_range and _attack_cooldown <= 0) then
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

   if (next_state ~= _state) then

      -- update state
      _prev_state = _state
      _state = next_state

      -- Log state change
      local state_names = {[STATE_IDLE] = "IDLE", [STATE_ATTACK] = "ATTACK", [STATE_DYING] = "DYING"}
      print("Frog: State changed from " .. state_names[_prev_state] .. " to " .. state_names[_state])

      -- reset the animation frame to 0 when the state changes
      _animation_frame = 0

      -- Reset tongue extension
      _tongue_scale = 0.0

      -- start cooldown after attack
      if _state == STATE_ATTACK then
         _attack_cooldown = _attack_delay
      end
   end

end

------------------------------------------------------------------------------------------------------------------------
function updateSprite(dt)

   local x = 0
   local y = 0

   -- Determine sprite coordinates based on current state
   if _state == STATE_DYING then
      x, y = getDyingSpriteCoords()
   elseif _state == STATE_ATTACK then
      x, y = getAttackSpriteCoords()
   else  -- STATE_IDLE
      x, y = getIdleSpriteCoords()
   end

   -- Log the sprite row being used (only when it changes significantly)
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

   -- update the main frog sprite
   updateSpriteRect(
      0,
      x,
      y,
      SPRITE_WIDTH,
      SPRITE_HEIGHT
   )

   -- handle state-specific updates
   if _state == STATE_IDLE then
      -- hide the tongue sprite when in idle state
      setSpriteVisible(1, false)  -- hide tongue sprite
   elseif _state == STATE_ATTACK then
      updateSpriteAttack(math.floor(_animation_frame) % FRAME_COUNTS[CYCLE_ATTACK], dt)
   elseif _state == STATE_DYING then
      -- Hide tongue during death animation
      setSpriteVisible(1, false)
   end
end


------------------------------------------------------------------------------------------------------------------------
-- function to handle attack state updates
function updateSpriteAttack(frame_index, dt)
   -- determine if we're showing the tongue based on the attack animation frame
   -- the tongue should appear during certain frames of the attack animation
   local show_tongue = frame_index >= 2 and frame_index <= 4  -- show tongue during middle frames of attack

   -- handle the case when tongue is not shown first and return early
   if not show_tongue then
      -- hide the tongue sprite when not attacking or tongue not visible
      setSpriteVisible(1, false)
      setSpriteScale(1, 0, 0)
      return
   end

   -- calculate distance to player to determine tongue length
   local dist_to_player = math.abs(_player_position:getX() - _position:getX())

   -- scale the tongue length based on distance to player (but cap it)
   local max_tongue_length = 100  -- max length in pixels
   local current_tongue_length = math.min(dist_to_player, max_tongue_length)

   -- define tongue properties with floating-point precision
   local tongue_width = current_tongue_length  -- stretch the tongue to the calculated length
   local tongue_height = 8.0                 -- fixed height for the tongue (float)

   -- calculate tongue position based on frog facing direction with floating-point precision
   local base_x, base_y
   if _points_left then
      -- tongue extends to the left
      base_x = _position:getX()
      base_y = _position:getY()
   else
      -- tongue extends to the right
      base_x = _position:getX()
      base_y = _position:getY()
   end

   -- update the tongue sprite dimensions once
   updateSpriteRect(
      1,                    -- sprite slot for stretched tongue
      240,                  -- x coordinate in sprite sheet for tongue
      80,                   -- y coordinate in sprite sheet for tongue
      24,                   -- width in sprite sheet (source)
      8                     -- height in sprite sheet (source)
   )

   -- position the tongue sprite using offset
   setSpriteOffset(1, base_x, base_y)

   -- scale the sprite over time from 0 to 5
   _tongue_scale = _tongue_scale + dt * 10  -- increment by dt * 10
   if _tongue_scale > 5.0 then
      _tongue_scale = 5.0  -- clamp at 5
   end

   -- apply the scale
   setSpriteScale(1, _tongue_scale, 1.0)  -- scale only horizontally

   -- show the tongue sprite
   setSpriteVisible(1, true)

   -- calculate the actual displayed tongue width based on scale
   local displayed_tongue_width = 24 * _tongue_scale  -- original width (24) scaled by current scale

   -- check for collision between tongue and player
   local tongue_rect = {
      x = base_x - (displayed_tongue_width / 2),
      y = base_y - 4,  -- half of original height (8)
      w = displayed_tongue_width,
      h = 8  -- original height
   }

   -- adjust the x position based on direction
   if _points_left then
      tongue_rect.x = base_x - displayed_tongue_width
   end

   local player_rect = {
      x = _player_position:getX() - 12,  -- assuming player is 24x24
      y = _player_position:getY() - 12,
      w = 24,
      h = 24
   }

   -- simple rectangle collision check with tongue
   if (tongue_rect.x < player_rect.x + player_rect.w and
       tongue_rect.x + tongue_rect.w > player_rect.x and
       tongue_rect.y < player_rect.y + player_rect.h and
       tongue_rect.y + tongue_rect.h > player_rect.y) then

       -- cause damage to player when tongue hits
       causeDamageToPlayer(properties.damage)
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

   -- make sure the thing stops to move
   if _key_pressed then
      keyReleased(Key["KeyLeft"])
      keyReleased(Key["KeyRight"])
   end

   -- when dying, stop causing damage to the player
   setDamage(0)
end

------------------------------------------------------------------------------------------------------------------------
function hit(damage_value)

   if (_dead or _state == STATE_DYING) then
      return
   end

   -- need to store the current hit time
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

   -- update key presses for movement
   updateKeysPressed(_key_pressed)
   
   -- If death animation is finished, remove the enemy
   if _death_animation_finished then
      -- This would typically trigger removal of the entity from the game
      -- The exact method depends on the engine's API
   end
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
      end
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
   
   -- calculate sprite coordinates
   local x = frame_index * SPRITE_WIDTH
   local y = (cycle - 1) * SPRITE_HEIGHT
   
   -- Apply alignment offset for non-dying states
   y = y + _alignment_offset
   
   return x, y
end

------------------------------------------------------------------------------------------------------------------------
function getAttackSpriteCoords()
   local cycle = CYCLE_ATTACK
   local max_frames = FRAME_COUNTS[cycle]
   
   local frame_index = math.floor(_animation_frame) % max_frames
   
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
   
   -- Calculate sprite coordinates for death animation
   local x = death_frame_index * SPRITE_WIDTH
   local y = DEATH_ROW * SPRITE_HEIGHT  -- Use the death row (9th row, counting from 0)
   
   -- Apply alignment offset for death animation (same for both left and right)
   y = y + _alignment_offset
   
   return x, y
end

------------------------------------------------------------------------------------------------------------------------
-- Function to update animation frames
function updateAnimationFrames(dt, anim_speed_factor, is_death_animation)
   if is_death_animation then
      _death_animation_frame = _death_animation_frame + dt * 10.0  -- 10 fps base rate for death animation
   else
      _animation_frame = _animation_frame + dt * 10.0 * anim_speed_factor  -- 10 fps base rate
   end
end

------------------------------------------------------------------------------------------------------------------------
-- Function to update blinking logic when an idle animation cycle completes
function updateBlinkingOnCycleComplete()
   -- Only handle blinking when in idle state and not already blinking
   if _state ~= STATE_IDLE or _is_blinking then
      return
   end

   -- note that during idle, the frog can also blink
   -- i'd just go by random and pick that animation cycle with a change from 1:20
   if math.random(1, 20) == 1 then
      _is_blinking = true
      _blink_timer = 0
      _animation_frame = 0  -- reset animation frame for blink
      print("Frog: Blinking started")
   end
end

------------------------------------------------------------------------------------------------------------------------
-- Function to update blinking state (when already blinking)
function updateBlinking()
   -- Only handle blinking when in idle state and currently blinking
   if _state ~= STATE_IDLE or not _is_blinking then
      return
   end

   -- handle blinking state - finish when last frame is reached
   -- check if we've reached the last frame of the blink animation
   local max_blink_frames = FRAME_COUNTS[CYCLE_BLINK]
   if _animation_frame >= max_blink_frames then
      _is_blinking = false
      _animation_frame = 0  -- reset frame counter after blinking
      print("Frog: Blinking stopped")
   end
end

------------------------------------------------------------------------------------------------------------------------
-- Function to update dying state
function updateStateDying(dt)
   -- Update death animation frame
   updateAnimationFrames(dt, 1.0, true)  -- 10 fps base rate for death animation
   local death_frame_index = math.floor(_death_animation_frame)

   -- Check if death animation is finished
   if death_frame_index >= FRAME_COUNTS[4] then  -- FRAME_COUNTS[4] corresponds to death animation frames
      if not _death_animation_finished then
         _death_animation_finished = true
         _dead = true  -- Mark as truly dead after animation finishes
         print("Frog: Death animation finished, frog is now dead")
      end
   end
   
   return STATE_DYING  -- Stay in dying state
end

------------------------------------------------------------------------------------------------------------------------
-- Function to update attack state
function updateStateAttack(dt)
   -- increase the current frame based on dt
   local anim_speed_factor = _attack_anim_speed  -- attack animation speed
   updateAnimationFrames(dt, anim_speed_factor, false)  -- 10 fps base rate
   
   -- Check if attack animation is complete
   local current_frame_idx = math.floor(_animation_frame)
   local max_frames = FRAME_COUNTS[CYCLE_ATTACK]
   
   -- If we've reached the last frame of the attack animation, return to idle
   if current_frame_idx >= max_frames then
      return STATE_IDLE
   else
      return STATE_ATTACK
   end
end

------------------------------------------------------------------------------------------------------------------------
-- Function to update idle state
function updateStateIdle(dt)
   -- increase the current frame based on dt
   local anim_speed_factor = _idle_anim_speed    -- idle animation speed
   updateAnimationFrames(dt, anim_speed_factor, false)  -- 10 fps base rate

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
-- Function to update blink state
function updateStateBlinking(dt)
   -- increase the current frame based on dt
   local anim_speed_factor = _blink_anim_speed   -- blink animation speed
   updateAnimationFrames(dt, anim_speed_factor, false)  -- 10 fps base rate

   -- Update blinking state (to check if blink animation is complete)
   updateBlinking()
   
   return STATE_IDLE  -- Return to idle state when blinking is done
end
