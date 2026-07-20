require "data/scripts/enemies/constants"
v2d = require "data/scripts/enemies/vectorial2"

-- frog_bubble = 2 x 2 x 24
--
-- same sprite sheet and size as frog, but attacks with a burst of bubbles
-- instead of a tongue when the mouth opens.
--
-- bubble burst:
--   when attack animation completes (mouth fully open), fires BUBBLE_COUNT bubbles
--   one every BUBBLE_INTERVAL seconds, each with a progressively steeper upward angle
--   and a slight reduction in horizontal speed, producing a spreading swirl pattern.
--   negative gravity_scale keeps them floating upward in flight.
--
-- sprite rows:
-- 1) idle left (8 frames)
-- 2) idle blink left (8 frames)
-- 3) attack left (6 frames)
-- 4) idle right (8 frames)
-- 5) idle blink right (8 frames)
-- 6) attack right (6 frames)
-- 7-9) unused
-- 10) dying animation (24 frames)

ROW_OFFSET_RIGHT = 3
FRAME_COUNTS = {8, 8, 6, 19}
DEBUG_ATTACK_OFFSET = 0 * 48

BURP_MODE_PLAYER_PROXIMITY = 1
BURP_MODE_FIXED_INTERVAL   = 2

BURP_MODE     = BURP_MODE_FIXED_INTERVAL
BURP_INTERVAL = 4.0   -- used by BURP_MODE_FIXED_INTERVAL
MIN_ATTACK_WAIT = 1.5  -- used by BURP_MODE_PLAYER_PROXIMITY

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

BUBBLE_COUNT      = 30   -- total bubbles per burp
BUBBLE_BURST_SIZE = 6    -- bubbles spawned simultaneously per burst event (= number of weapon slots)
BUBBLE_INTERVAL   = 0.10 -- seconds between burst events

-- derived: number of burst events = ceil(BUBBLE_COUNT / BUBBLE_BURST_SIZE)
BURST_EVENT_COUNT = math.ceil(BUBBLE_COUNT / BUBBLE_BURST_SIZE)

BUBBLE_SPEED_H        = 0.05  -- base horizontal impulse
BUBBLE_SPEED_H_SPREAD = 0.015 -- random ± spread on horizontal impulse
BUBBLE_SPEED_V        = 0.003 -- base upward impulse (~11 deg slope at base speed)
BUBBLE_SPEED_V_SPREAD = 0.008 -- random spread added to upward impulse (always positive)
BUBBLE_SPAWN_SPREAD_X = 6     -- random ± pixel spread on spawn x
BUBBLE_SPAWN_SPREAD_Y = 8     -- random ± pixel spread on spawn y


------------------------------------------------------------------------------------------------------------------------
-- member variables
_done = false
_position = v2d.Vector2D(0, 0)
_player_position = v2d.Vector2D(0, 0)
_elapsed = 0
_alignment_offset = 0
_state = STATE_IDLE
_points_left = true
_direction_multiplier = -1  -- -1 for left, 1 for right
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
_attack_delay = 1.0
_attack_animation_time = 0.0
_attack_animation_speed = 10.0
_backward_animation_completed = false
_burst_end_time = nil       -- _attack_animation_time when burst finished
_last_attack_time = 0.0

-- bubble burst state
_burst_active = false        -- currently emitting bubbles
_burst_bubble_index = 0      -- next bubble to fire (0-based)
_burst_timer = 0.0           -- time accumulator for BUBBLE_INTERVAL
_burst_all_fired = false     -- all BUBBLE_COUNT bubbles have been launched

-- health and death variables
_energy = 30
_dead = false
_key_pressed = 0

-- dying animation variables
_death_animation_frame = 0
_death_animation_finished = false

-- animation tracking
_prev_log_sprite_row = nil
_prev_animation_frame = 0


------------------------------------------------------------------------------------------------------------------------
properties = {
   sprite = "data/sprites/enemy_frog.png",
   damage = 0,           -- bubble projectiles carry their own damage; the body itself is harmless
   smash = true,
   velocity_walk_max = 0.0,
   acceleration_ground = 0.0
}


------------------------------------------------------------------------------------------------------------------------
function initialize()
   addShapeRect(0.4, 0.25, 0.0, 0.05)

   addHitbox(-18, -18, 36, 36)

   -- one weapon slot per simultaneous bubble; each slot has its own fire clock so all
   -- BUBBLE_BURST_SIZE slots can fire in the same update without being blocked by the interval.
   -- frames_per_row=132 is floor(3176/24); start_frame=39*132+25=5173 maps to pixel (600,936).
   for slot = 0, BUBBLE_BURST_SIZE - 1 do
      addWeapon(WeaponType["Gun"], 1, 1, -0.05, 0.08)
      updateProjectileAnimation(
         slot,
         "data/sprites/player.png",
         24,     -- frame_width
         24,     -- frame_height
         12,     -- frame_origin_x
         12,     -- frame_origin_y
         1.05,   -- time_per_frame_s
         3,      -- frame_count
         132,    -- frames_per_row
         5173    -- start_frame: maps to pixel (600, 936)
      )
   end

   updateSprite(0.0)
end


------------------------------------------------------------------------------------------------------------------------
function checkAttackCondition(next_state)

   if next_state == _state and _state ~= STATE_DYING and _state ~= STATE_ATTACK then

      if BURP_MODE == BURP_MODE_FIXED_INTERVAL then

         if _elapsed - _last_attack_time >= BURP_INTERVAL then
            next_state = STATE_ATTACK
         end

      elseif BURP_MODE == BURP_MODE_PLAYER_PROXIMITY then

         local x_diff = _player_position:getX() // 24 - _position:getX() // 24
         local y_diff = _player_position:getY() // 24 - _position:getY() // 24

         local x_in_range =
            (_points_left and x_diff >= -5 and x_diff <= 0) or
            (not _points_left and x_diff <= 5 and x_diff >= 0)

         local y_in_range = y_diff >= -1 and y_diff <= 1

         if y_in_range and x_in_range and _elapsed - _last_attack_time >= MIN_ATTACK_WAIT then
            if isPhsyicsPathClear(
               _position:getX(), _position:getY(),
               _player_position:getX(), _player_position:getY()
            ) then
               next_state = STATE_ATTACK
            end
         end

      end

   end

   return next_state
end


------------------------------------------------------------------------------------------------------------------------
function resetOnStateTransition()
   _animation_frame = 0
   _attack_animation_time = 0.0
   _backward_animation_completed = false
   _burst_end_time = nil
   _burst_active = false
   _burst_bubble_index = 0
   _burst_timer = 0.0
   _burst_all_fired = false
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
      print("FrogBubble: State changed from " .. state_names[_prev_state] .. " to " .. state_names[_state])

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

   updateSpriteRect(0, x, y, width, height)

   if _state == STATE_ATTACK then
      updateSpriteAttack(dt)
   end
end


------------------------------------------------------------------------------------------------------------------------
-- fires one burst event: all BUBBLE_BURST_SIZE slots simultaneously with randomized
-- velocities and spawn positions, matching the WaterBubbles scatter pattern.
local function fireBubble()
   local spawn_x_base = _position:getX() + _direction_multiplier * 20
   local spawn_y_base = _position:getY()

   for slot = 0, BUBBLE_BURST_SIZE - 1 do
      local horiz  = BUBBLE_SPEED_H + (math.random() - 0.5) * 2.0 * BUBBLE_SPEED_H_SPREAD
      local upward = BUBBLE_SPEED_V + math.random() * BUBBLE_SPEED_V_SPREAD
      local spawn_x = spawn_x_base + (math.random() - 0.5) * 2.0 * BUBBLE_SPAWN_SPREAD_X
      local spawn_y = spawn_y_base + (math.random() - 0.5) * 2.0 * BUBBLE_SPAWN_SPREAD_Y
      useWeapon(slot, spawn_x, spawn_y, _direction_multiplier * horiz, -upward)
   end
end


------------------------------------------------------------------------------------------------------------------------
-- advances the bubble burst and fires pending bubbles
function updateSpriteAttack(dt)
   local current_frame_index = math.floor(_attack_animation_time * _attack_animation_speed * _attack_anim_speed)
   local max_frames = FRAME_COUNTS[CYCLE_ATTACK] - 1
   local animation_completed = current_frame_index >= max_frames

   if animation_completed and not _burst_all_fired then
      if not _burst_active then
         _burst_active = true
         _burst_bubble_index = 0
         _burst_timer = 0.0
      end

      _burst_timer = _burst_timer + dt

      while _burst_timer >= BUBBLE_INTERVAL and _burst_bubble_index < BURST_EVENT_COUNT do
         fireBubble()
         _burst_bubble_index = _burst_bubble_index + 1
         _burst_timer = _burst_timer - BUBBLE_INTERVAL
      end

      if _burst_bubble_index >= BURST_EVENT_COUNT then
         _burst_all_fired = true
         _burst_active = false
         if not _burst_end_time then
            _burst_end_time = _attack_animation_time
         end
      end
   end
end

------------------------------------------------------------------------------------------------------------------------
function smashed()

   if (_smashed or _state == STATE_DYING) then
      return
   end

   print("FrogBubble: Smashed, starting to die")

   _smashed = true
   startDying()
end


------------------------------------------------------------------------------------------------------------------------
function startDying()
   _state = STATE_DYING
   _death_animation_frame = 0
   _elapsed = 0.0

   setDamage(0)
end


------------------------------------------------------------------------------------------------------------------------
function hit(damage_value)

   if (_dead or _state == STATE_DYING) then
      return
   end

   _energy = _energy - damage_value
   print("FrogBubble: Hit for " .. damage_value .. " damage, remaining energy: " .. _energy)

   if (_energy <= 0) then
      print("FrogBubble: Energy depleted, starting to die")
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
         _direction_multiplier = 1
      else
         _direction_multiplier = -1
         _alignment_offset = 0
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
function getIdleSpriteCoords()
   local cycle = CYCLE_IDLE
   local max_frames = FRAME_COUNTS[cycle]

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

   local frame_index = math.floor(_attack_animation_time * _attack_animation_speed * _attack_anim_speed)

   if _burst_all_fired and not _backward_animation_completed then
      -- play close-mouth animation backwards after all bubbles have been fired
      local time_since_burst_end = math.max(0, _attack_animation_time - (_burst_end_time or _attack_animation_time))
      local frames_since_burst_end = math.floor(time_since_burst_end * _attack_animation_speed * _attack_anim_speed)
      frame_index = math.max(0, max_frames - frames_since_burst_end)
      if frame_index == 0 then
         _backward_animation_completed = true
      end
   elseif _burst_all_fired and _backward_animation_completed then
      -- stay at frame 0 (mouth closed) until state transitions away
      frame_index = 0
   elseif _burst_active or (frame_index >= max_frames and not _burst_all_fired) then
      -- hold on the last frame (mouth open) while the burst is in progress
      frame_index = max_frames
   else
      -- forward animation: mouth opening, capped at the final open-mouth frame
      frame_index = math.min(frame_index, max_frames)
   end

   local x = frame_index * SPRITE_WIDTH + DEBUG_ATTACK_OFFSET
   local y = (cycle - 1) * SPRITE_HEIGHT + _alignment_offset

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
function updateStateDying(dt)
   _death_animation_frame = _death_animation_frame + dt * _death_anim_speed
   local death_frame_index = math.floor(_death_animation_frame)

   if death_frame_index >= FRAME_COUNTS[CYCLE_DYING] then
      if not _death_animation_finished then
         _death_animation_finished = true
         _dead = true
         print("FrogBubble: Death animation finished")
      end
   end

   return STATE_DYING
end

------------------------------------------------------------------------------------------------------------------------
function updateStateAttack(dt)
   _attack_animation_time = _attack_animation_time + dt

   -- stay in ATTACK until the close-mouth backward animation has completed
   if _burst_all_fired and _backward_animation_completed then
      return STATE_IDLE
   else
      return STATE_ATTACK
   end
end

------------------------------------------------------------------------------------------------------------------------
function updateStateIdle(dt)
   _animation_frame = _animation_frame + dt * _idle_anim_speed

   local current_frame_idx = math.floor(_animation_frame) % FRAME_COUNTS[CYCLE_IDLE]
   local prev_frame_idx    = math.floor(_prev_animation_frame) % FRAME_COUNTS[CYCLE_IDLE]

   if prev_frame_idx > current_frame_idx and prev_frame_idx >= FRAME_COUNTS[CYCLE_IDLE] - 1 then
      _is_blinking = math.random(1, 20) == 1
   end

   _prev_animation_frame = _animation_frame

   return STATE_IDLE
end
