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
properties = {
   sprite = "data/sprites/enemy_frog.png",
   damage = 4,
   smash = true,
   velocity_walk_max = 0.0,  -- frogs don't walk, so set to 0
   acceleration_ground = 0.0  -- frogs don't accelerate, so set to 0
}


------------------------------------------------------------------------------------------------------------------------
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

   -- Don't update state if the frog is dying
   if _state == STATE_DYING then
      return
   end

   x_diff = _player_position:getX() // 24 - _position:getX() // 24
   y_diff = _player_position:getY() // 24 - _position:getY() // 24

   x_in_range =
      (_points_left and x_diff >= -5 and x_diff <= 0) or
      (not _points_left and x_diff <= 5 and x_diff >= 0)

   y_in_range = y_diff >= -1 and y_diff <= 1

   next_state = STATE_IDLE

   -- check for attack transition
   if (y_in_range and x_in_range and _attack_cooldown <= 0) then
      if (
         isPhsyicsPathClear(
            _position:getX(),
            _position:getY(),
            _player_position:getX(),
            _player_position:getY()
         )
      ) then
         print(x_diff)
         print("attack")
         next_state = STATE_ATTACK
      end
   end

   -- reset the animation frame to 0 when the state changes

   if (next_state ~= _state) then
      _prev_state = _state
      _state = next_state
      _animation_frame = 0

      -- start cooldown after attack
      if _state == STATE_ATTACK then
         _attack_cooldown = _attack_delay
         -- Start tongue extension when entering attack state
         _tongue_scale = 0.0
      else
         -- Reset tongue extension when leaving attack state
         _tongue_scale = 0.0
      end
   end

   -- update cooldown timer
   if _attack_cooldown > 0 then
      _attack_cooldown = _attack_cooldown - dt
      if _attack_cooldown < 0 then
         _attack_cooldown = 0
      end
   end

end

------------------------------------------------------------------------------------------------------------------------
function updateSprite(dt)

   x = 0
   y = 0

   -- Handle dying state separately
   if _state == STATE_DYING then
      -- Update death animation frame
      _death_animation_frame = _death_animation_frame + dt * 10.0  -- 10 fps base rate for death animation
      local death_frame_index = math.floor(_death_animation_frame)

      -- Check if death animation is finished
      if death_frame_index >= FRAME_COUNTS[4] then  -- FRAME_COUNTS[4] corresponds to death animation frames
         _death_animation_finished = true
         _dead = true  -- Mark as truly dead after animation finishes
      else
         -- Calculate sprite coordinates for death animation
         x = death_frame_index * SPRITE_WIDTH
         y = DEATH_ROW * SPRITE_HEIGHT  -- Use the death row (9th row, counting from 0)
         
         -- Apply alignment offset for death animation (same for both left and right)
         y = y + _alignment_offset
      end
   else
      -- note that during idle, the frog can also blink
      -- i'd just go by random and pick that animation cycle with a change from 1:20
      if _state == STATE_IDLE and not _is_blinking then
         -- random chance to start blinking (1 in 20 per second)
         if math.random(1, 20) == 1 then
            _is_blinking = true
            _blink_timer = 0
            _animation_frame = 0  -- reset animation frame for blink
         end
      end

      -- handle blinking state - finish when last frame is reached
      if _is_blinking and _state == STATE_IDLE then
         -- check if we've reached the last frame of the blink animation
         local max_blink_frames = FRAME_COUNTS[CYCLE_BLINK]
         if _animation_frame >= max_blink_frames then
            _is_blinking = false
            _animation_frame = 0  -- reset frame counter after blinking
         end
      end

      -- set the animation frame based on the state
      local cycle = CYCLE_IDLE
      local max_frames = FRAME_COUNTS[cycle]

      -- determine the animation cycle based on state and blinking
      if _state == STATE_ATTACK then
         cycle = CYCLE_ATTACK
         max_frames = FRAME_COUNTS[cycle]
      elseif _is_blinking then
         cycle = CYCLE_BLINK
         max_frames = FRAME_COUNTS[cycle]
      else
         -- for idle state
         cycle = CYCLE_IDLE
         max_frames = FRAME_COUNTS[cycle]
      end

      -- increase the current frame based on dt
      -- each animation should have its own animation_speed_factor that is multiplied into the calculation
      -- default is 1 for each
      local anim_speed_factor
      if _state == STATE_ATTACK then
         anim_speed_factor = _attack_anim_speed  -- attack animation speed
      elseif _is_blinking then
         anim_speed_factor = _blink_anim_speed   -- blink animation speed
      else
         anim_speed_factor = _idle_anim_speed    -- idle animation speed
      end

      _animation_frame = _animation_frame + dt * 10.0 * anim_speed_factor  -- 10 fps base rate
      local frame_index = math.floor(_animation_frame) % max_frames

      -- calculate sprite coordinates
      x = frame_index * SPRITE_WIDTH
      y = (cycle - 1) * SPRITE_HEIGHT

      -- Apply attack sprite offset when attacking
      if _state == STATE_ATTACK then
         x = x + DEBUG_ATTACK_OFFSET
      end

      -- Apply alignment offset for non-dying states
      y = y + _alignment_offset
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
      updateSpriteAttack(math.floor(_animation_frame) % FRAME_COUNTS[CYCLE_ATTACK])
   elseif _state == STATE_DYING then
      -- Hide tongue during death animation
      setSpriteVisible(1, false)
   end
end


-- function to handle attack state updates
function updateSpriteAttack(frame_index)
   -- determine if we're showing the tongue based on the attack animation frame
   -- the tongue should appear during certain frames of the attack animation
   local show_tongue = frame_index >= 2 and frame_index <= 4  -- show tongue during middle frames of attack

   if show_tongue then
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

      -- position the tongue sprite
      setSpritePosition(1, base_x, base_y)

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
   else
      -- hide the tongue sprite when not attacking or tongue not visible
      setSpriteVisible(1, false)
      setSpriteScale(1, 0, 0)
   end
end

------------------------------------------------------------------------------------------------------------------------
function smashed()

   if (_smashed or _state == STATE_DYING) then
      return
   end

   _smashed = true

   startDying()
end

------------------------------------------------------------------------------------------------------------------------
function startDying()
   _state = STATE_DYING
   _death_animation_frame = 0
   _elapsed = 0.0

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
   if (_energy <= 0) then
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

function keyReleased(key)
   _key_pressed = _key_pressed & (~key)
end

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
