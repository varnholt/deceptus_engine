require "data/scripts/enemies/constants"
v2d = require "data/scripts/enemies/vectorial2"

-- blue fire frog = 2 x 2 x 24
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
-- blue frog uses the same animation rows as the standard frog, shifted horizontally on the sprite sheet.
-- Instead of extending a tongue after the mouth animation, it shoots a blue fireball from its mouth.
--
-- blue fireball/orb animation row starts at y = 336.
--
-- Exposed map/editor parameters you can test in-game:
--
-- alignment=left                  -- left or right
-- attack_interval=3.0             -- seconds between shots
-- orb_path=straight               -- straight or sine
-- orb_speed=160.0                 -- fireball travel speed
-- orb_lifetime=3000               -- lifetime in milliseconds before disappearing
-- wave_strength=24.0              -- vertical size for sine movement
-- wave_speed=8.0                  -- sine frequency/speed
-- idle_anim_speed=10.0
-- attack_anim_speed=2.0
-- blink_anim_speed=1.0

ROW_OFFSET_RIGHT = 3
FRAME_COUNTS = {8, 8, 6, 19}
BLUE_FROG_X_OFFSET = 768  -- Blue frog frames start here on the sprite sheet
DEBUG_ATTACK_OFFSET = 0 * 48  -- extra debug x offset when attacking

DEFAULT_ATTACK_INTERVAL = 3.0  -- Shoot automatically every 3 seconds, regardless of player range

DEFAULT_FIREBALL_SPEED = 160.0
DEFAULT_ORB_LIFETIME = 3000  -- lifetime in milliseconds before disappearing

DEFAULT_ORB_PATH = "straight"
DEFAULT_WAVE_STRENGTH = 24.0
DEFAULT_WAVE_SPEED = 8.0
-- First visible fireball position. Tweaked so the orb appears right at the blue frog mouth,
-- not one full orb-width away from the body.
FIREBALL_START_OFFSET_LEFT = 18
FIREBALL_START_OFFSET_RIGHT =28
FIREBALL_START_OFFSET_Y = 0
FIREBALL_WIDTH = 120
FIREBALL_HEIGHT = 120
FIREBALL_ROW_Y = 312
FIREBALL_FRAME_SPACING = 120
FIREBALL_FRAME_COUNT = 8
-- Shooting animation timing. The blue frog frames are 48x48.
-- The blue frog starts at grid column 17 on the shared frog sprite sheet.
-- For the attack row, this maps to these grid cells:
--   pre-fire: 17,3 -> 19,3 quickly, then hold on 19,3 for the rest of the 1-second windup
--   fire:     20,3 -> 22,3, and the orb becomes visible at 20,3
--   close:    22,3 -> 21,3 -> 20,3 -> 17,3
-- The same horizontal frame sequence is used on the right-facing attack row.
ATTACK_WINDUP_TIME = 1.0        -- whole windup step duration
ATTACK_WINDUP_OPEN_TIME = 0.025  -- mouth opens twice as fast; smaller = faster
ATTACK_FIRE_FRAME_TIME = 0.02
ATTACK_FIRE_HOLD_TIME = 0.30
ATTACK_CLOSE_FRAME_TIME = 0.10
ATTACK_WINDUP_FRAMES = {0, 1, 2}
ATTACK_FIRE_FRAMES = {3, 4, 5}
ATTACK_CLOSE_FRAMES = {5, 4, 3, 0}
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
_fireball_direction_multiplier = -1  -- -1 for left, 1 for right
_smashed = false
_animation_frame = 0
_prev_state = STATE_IDLE
_is_blinking = false

-- animation speed factors for different states
_death_anim_speed = 10.0
_idle_anim_speed = 10.0
_attack_anim_speed = 2.0
_blink_anim_speed = 1.0

-- attack state variables
_attack_interval = DEFAULT_ATTACK_INTERVAL
_fireball_active = false          -- Whether the fireball is currently visible/travelling
_fireball_distance = 0.0          -- Distance travelled by the current fireball
_orb_elapsed_ms = 0.0             -- Current orb lifetime timer in milliseconds
_fireball_hit_player = false      -- Flag to track if fireball has hit the player
_retracting_tongue = false        -- Re-used as the attack closing phase flag
_attack_animation_time = 0.0      -- Time elapsed in the current attack animation phase
_attack_animation_time_at_full_retraction = nil  -- Time when tongue became fully retracted
_backward_animation_completed = false  -- Flag to track if backward animation has completed
_fireball_fired_time = nil          -- Attack animation time when the orb was fired
_attack_animation_speed = 10.0
_tongue_speed = 10.0  -- kept for compatibility with external tuning; fireball uses _orb_speed
_last_attack_time = 0.0  -- Time of the last attack

-- fireball/orb path tuning
_orb_path = DEFAULT_ORB_PATH
_orb_speed = DEFAULT_FIREBALL_SPEED
_orb_lifetime = DEFAULT_ORB_LIFETIME
_wave_strength = DEFAULT_WAVE_STRENGTH
_wave_speed = DEFAULT_WAVE_SPEED

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

-- fireball sprite dimensions
_fireball_sprite_x = 0
_fireball_sprite_y = FIREBALL_ROW_Y


------------------------------------------------------------------------------------------------------------------------
properties = {
   sprite = "data/sprites/enemy_frog.png",
   damage = 1,
   smash = true,
   velocity_walk_max = 0.0,  -- frogs don't walk, so set to 0
   acceleration_ground = 0.0,  -- frogs don't accelerate, so set to 0

   -- attack timing
   attack_interval = DEFAULT_ATTACK_INTERVAL,

   -- travelling orb behaviour
   orb_path = DEFAULT_ORB_PATH,
   orb_speed = DEFAULT_FIREBALL_SPEED,
   orb_lifetime = DEFAULT_ORB_LIFETIME,

   -- sine path tuning
   wave_strength = DEFAULT_WAVE_STRENGTH,
   wave_speed = DEFAULT_WAVE_SPEED
}


------------------------------------------------------------------------------------------------------------------------
function initialize()
   addShapeRect(0.4, 0.25, 0.0, 0.05)
   -- setSpriteOffset(0, 0, 24);

   -- add hitbox for damage detection
   addHitbox(-18, -18, 16, 16)  -- x offset, y offset, width, height

   -- sprite slot for the travelling blue fireball (hidden by default)
   addSprite()
   setSpriteOffset(1, 36, -12)
   setSpriteScale(1, 1.0, 1.0)
   setSpriteVisible(1, false)

   updateSprite(0.0)
end


------------------------------------------------------------------------------------------------------------------------
function checkAttackCondition(next_state)

   -- Auto-shoot every _attack_interval seconds.
   -- This intentionally ignores player range and line-of-sight checks.
   if next_state == _state and _state ~= STATE_DYING and _state ~= STATE_ATTACK then
      local time_since_last_attack = _elapsed - _last_attack_time

      if time_since_last_attack >= _attack_interval then
         print("Frog: Auto attack triggered")
         next_state = STATE_ATTACK
      end
   end

   return next_state
end

------------------------------------------------------------------------------------------------------------------------
function resetOnStateTransition(next_state, previous_state)
   _animation_frame = 0

   -- Keep an already-fired orb alive when the frog leaves ATTACK.
   -- This lets the frog close its mouth and go back to idle while the orb keeps travelling.
   local keep_fireball = _fireball_active and previous_state == STATE_ATTACK and next_state == STATE_IDLE

   if not keep_fireball then
      _fireball_active = false
      _fireball_distance = 0.0
      _orb_elapsed_ms = 0.0
      _fireball_hit_player = false
      setSpriteVisible(1, false)
   end

   _retracting_tongue = false
   _attack_animation_time = 0.0
   _attack_animation_time_at_full_retraction = nil
   _backward_animation_completed = false
   _fireball_fired_time = nil
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

      resetOnStateTransition(next_state, _prev_state)

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

   updateFireball(dt)
end


------------------------------------------------------------------------------------------------------------------------
function isValidOrbPath(value)
   return (value == "straight" or value == "sine")
end

------------------------------------------------------------------------------------------------------------------------
function getFireballPathOffsetY()
   local base_y = FIREBALL_START_OFFSET_Y

   if _orb_path == "sine" then
      return base_y + math.sin(_fireball_distance * 0.05 * _wave_speed) * _wave_strength
   end

   return base_y
end

------------------------------------------------------------------------------------------------------------------------
-- update the travelling fireball. This runs in both ATTACK and IDLE so the frog can
-- close its mouth immediately after firing instead of holding the open-mouth frame.
function updateFireball(dt)
   if not _fireball_active then
      setSpriteVisible(1, false)
      return
   end

   _fireball_distance = _fireball_distance + dt * _orb_speed
   _orb_elapsed_ms = _orb_elapsed_ms + dt * 1000.0

   local fireball_frame = math.floor(_elapsed * 18.0) % FIREBALL_FRAME_COUNT
   updateSpriteRect(
      1,
      _fireball_sprite_x + fireball_frame * FIREBALL_FRAME_SPACING,
      _fireball_sprite_y,
      FIREBALL_WIDTH,
      FIREBALL_HEIGHT
   )

   local fireball_offset_x
   if _points_left then
      fireball_offset_x = FIREBALL_START_OFFSET_LEFT - _fireball_distance
   else
      fireball_offset_x = FIREBALL_START_OFFSET_RIGHT + _fireball_distance
   end

   local fireball_offset_y = getFireballPathOffsetY()
   setSpriteOffset(1, fireball_offset_x, fireball_offset_y)
   setSpriteScale(1, 1.0, 1.0)
   setSpriteVisible(1, true)

   -- Use a smaller hit area inside the orb so the visual feels fair.
   local hitbox_x = _position:getX() + fireball_offset_x + 18
   local hitbox_y = _position:getY() + fireball_offset_y + 18
   if not _fireball_hit_player and intersectsWithPlayer(hitbox_x, hitbox_y, 36, 36) then
      damage(properties.damage, 0, 0)
      _fireball_hit_player = true
   end

   if _orb_elapsed_ms >= _orb_lifetime or _fireball_hit_player then
      _fireball_active = false
      setSpriteVisible(1, false)
   end
end

------------------------------------------------------------------------------------------------------------------------
-- function to handle attack state updates
function updateSpriteAttack(dt)
   -- Show the orb as soon as the shooting frames begin.
   -- The attack timeline is controlled by getAttackSpriteCoords(), so this function
   -- only starts the travelling fireball at the exact transition from windup to fire.
   if _attack_animation_time >= ATTACK_WINDUP_TIME and not _fireball_active and _fireball_fired_time == nil then
      _fireball_active = true
      _fireball_distance = 0.0
      _orb_elapsed_ms = 0.0
      _fireball_hit_player = false
      _fireball_fired_time = _attack_animation_time
   end
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
   setSpriteOffset(1, 36, -12)
   setSpriteVisible(1, false) -- hide the fireball

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
         _fireball_direction_multiplier = 1  -- 1 for right
      else
         _alignment_offset = 0
         _points_left = true
         _fireball_direction_multiplier = -1  -- -1 for left (default)
      end

      updateSpriteRect(
         1,
         _fireball_sprite_x,
         _fireball_sprite_y,
         FIREBALL_WIDTH,
         FIREBALL_HEIGHT
      )
      setSpriteVisible(1, false)

   elseif (key == "idle_anim_speed") then
      local v = tonumber(value)
      if (v ~= nil and v > 0) then
         _idle_anim_speed = v
      end

   elseif (key == "attack_anim_speed") then
      local v = tonumber(value)
      if (v ~= nil and v > 0) then
         _attack_anim_speed = v
      end

   elseif (key == "blink_anim_speed") then
      local v = tonumber(value)
      if (v ~= nil and v > 0) then
         _blink_anim_speed = v
      end

   elseif (key == "attack_interval" or key == "attack_delay") then
      -- attack_delay is kept as an alias, but attack_interval is the recommended name.
      local v = tonumber(value)
      if (v ~= nil and v > 0) then
         _attack_interval = v
         properties.attack_interval = v
      end

   elseif (key == "orb_path") then
      if isValidOrbPath(value) then
         _orb_path = value
         properties.orb_path = value
      else
         _orb_path = "straight"
         properties.orb_path = "straight"
      end

   elseif (key == "orb_speed") then
      local v = tonumber(value)
      if (v ~= nil and v > 0) then
         _orb_speed = v
         properties.orb_speed = v
      end

   elseif (key == "orb_lifetime" or key == "fireball_lifetime") then
      -- Lifetime is measured in milliseconds. Example: 3000 = 3 seconds.
      local v = tonumber(value)
      if (v ~= nil and v > 0) then
         _orb_lifetime = v
         properties.orb_lifetime = v
      end

   elseif (key == "wave_strength") then
      local v = tonumber(value)
      if (v ~= nil) then
         _wave_strength = v
         properties.wave_strength = v
      end

   elseif (key == "wave_speed") then
      local v = tonumber(value)
      if (v ~= nil) then
         _wave_speed = v
         properties.wave_speed = v
      end
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
   local x = BLUE_FROG_X_OFFSET + frame_index * SPRITE_WIDTH
   local y = (cycle - 1) * SPRITE_HEIGHT
   y = y + _alignment_offset
   
   return x, y, SPRITE_WIDTH, SPRITE_WIDTH
end

------------------------------------------------------------------------------------------------------------------------
function getAttackSpriteCoords()
   -- Attack frame sequence requested for the 48x48 sprite grid:
   --   1) windup for 1 second: frames 17,3 -> 19,3
   --   2) fire/show orb:       frames 20,3 -> 22,3
   --   3) close mouth:         frames 22,3 -> 21,3 -> 20,3 -> 17,3
   -- For the right-facing frog, _alignment_offset moves this same frame sequence
   -- down to the right-facing attack row.
   local frame_index = 0
   local t = _attack_animation_time

   if t < ATTACK_WINDUP_TIME then
      -- Open the mouth quickly using frames 17,3 -> 17,4 -> 17,5,
      -- then hold on 17,5 for the rest of the windup step.
      local step_time = ATTACK_WINDUP_OPEN_TIME / #ATTACK_WINDUP_FRAMES
      local i = math.floor(t / step_time) + 1
      if i < 1 then i = 1 end
      if i > #ATTACK_WINDUP_FRAMES then i = #ATTACK_WINDUP_FRAMES end
      frame_index = ATTACK_WINDUP_FRAMES[i]

   elseif t < ATTACK_WINDUP_TIME + (#ATTACK_FIRE_FRAMES * ATTACK_FIRE_FRAME_TIME) + ATTACK_FIRE_HOLD_TIME then
      local fire_t = t - ATTACK_WINDUP_TIME
      if fire_t >= (#ATTACK_FIRE_FRAMES * ATTACK_FIRE_FRAME_TIME) then
         frame_index = ATTACK_FIRE_FRAMES[#ATTACK_FIRE_FRAMES]
      else
         local i = math.floor(fire_t / ATTACK_FIRE_FRAME_TIME) + 1
         if i < 1 then i = 1 end
         if i > #ATTACK_FIRE_FRAMES then i = #ATTACK_FIRE_FRAMES end
         frame_index = ATTACK_FIRE_FRAMES[i]
      end

   else
      _retracting_tongue = true
      local close_t = t - ATTACK_WINDUP_TIME - (#ATTACK_FIRE_FRAMES * ATTACK_FIRE_FRAME_TIME) - ATTACK_FIRE_HOLD_TIME
      local i = math.floor(close_t / ATTACK_CLOSE_FRAME_TIME) + 1
      if i < 1 then i = 1 end
      if i > #ATTACK_CLOSE_FRAMES then
         i = #ATTACK_CLOSE_FRAMES
         _backward_animation_completed = true
      end
      frame_index = ATTACK_CLOSE_FRAMES[i]
   end

   local x = BLUE_FROG_X_OFFSET + frame_index * SPRITE_WIDTH + DEBUG_ATTACK_OFFSET
   local y = (CYCLE_ATTACK - 1) * SPRITE_HEIGHT + _alignment_offset

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
   _death_animation_frame = _death_animation_frame + dt * _death_anim_speed
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

   -- Return to idle once the mouth-closing animation is done.
   -- The fireball may still be visible/travelling; updateFireball() keeps it moving.
   if _retracting_tongue and _backward_animation_completed then
      return STATE_IDLE
   else
      return STATE_ATTACK
   end
end

------------------------------------------------------------------------------------------------------------------------
-- function to update idle state
function updateStateIdle(dt)
   _animation_frame = _animation_frame + dt * _idle_anim_speed

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


