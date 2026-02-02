------------------------------------------------------------------------------------------------------------------------
require "data/scripts/enemies/constants"
require "data/scripts/enemies/interpolation"
v2d = require "data/scripts/enemies/vectorial2"

------------------------------------------------------------------------------------------------------------------------
-- interpolation keys (kept for compatibility)
Key = {x = 0, y = 0, time = 0}
function Key:create(o)
  o.parent = self
  return o
end

------------------------------------------------------------------------------------------------------------------------
properties = {
   static_body = false,
   sprite = "data/sprites/enemy_bat_2.png",
   damage = 1
}

------------------------------------------------------------------------------------------------------------------------
_position_px = v2d.Vector2D(0, 0)
_position_at_death_px = v2d.Vector2D(0, 0)
_player_position_px = v2d.Vector2D(0, 0)
_player_position_previous_px = v2d.Vector2D(0, 0)

_elapsed = math.random(0, 3) -- also acts as per-bat phase seed
_activated = false

_sprite_offset_x = 0
_sprite_offset_y = 0
_sprite_width = 72
_sprite_height = 72

_start_position_px = v2d.Vector2D(0, 0)
_dying = false
_dead = false
_death_time = 0

_can_explode = false
_exploded = false
_energy = 3

ANIMATION_SPEED_IDLE = 20.0
ANIMATION_SPEED_DEATH = 20.0
HIT_RADIUS = 0.3
ATTACK_SPRITE_COUNT = 9

------------------------------------------------------------------------------------------------------------------------
-- STATES: no attack, only hover + follow + smooth return
STATE_IDLE   = 0
STATE_FOLLOW = 1
STATE_RETURN = 2

_state = STATE_IDLE
_state_time = 0.0

_pos = v2d.Vector2D(0, 0)  -- internally driven position
_vel = v2d.Vector2D(0, 0)

------------------------------------------------------------------------------------------------------------------------
-- Hover tuning (idle)
HOVER_OX_AMPL = 6.0
HOVER_OY_AMPL = 12.0
HOVER_OX_FREQ = 1.7
HOVER_OY_FREQ = 2.3
HOVER_EASE = 7.0
IDLE_HOVER_BLEND_TIME = 0.35

------------------------------------------------------------------------------------------------------------------------
-- Aggro / range + leash
-- Aggro is based on player distance to BASE (prevents dragging across map)
-- Leash clamps follow target so bat never exceeds its base roam radius.
AGGRO_X_TILES = 7
AGGRO_Y_TILES = 8
LEASH_X_TILES = 8
LEASH_Y_TILES = 9

------------------------------------------------------------------------------------------------------------------------
-- Follow tuning (feel)
FOLLOW_SPEED = 105.0
FOLLOW_TURN  = 4.5
FOLLOW_OFFSET_Y = -28.0
FOLLOW_LEAD = 3.0

-- When bat gets close to its follow target, make it "orbit" / flutter instead of sticking to 1 pixel
ORBIT_START_DIST = 18.0    -- start fluttering when closer than this (px)
ORBIT_RADIUS = 62.0        -- how wide it circles (px)
ORBIT_FREQ = 8.4           -- radians/sec-ish (bigger = faster orbit)
ORBIT_MIX = 0.85           -- 0..1, how much of orbit offset to apply when very close

------------------------------------------------------------------------------------------------------------------------
-- Return tuning (smooth drift back, with deceleration near home)
RETURN_SPEED = 65.0
RETURN_TURN  = 2.5
RETURN_SLOW_RADIUS = 18.0
RETURN_DONE_DIST  = 0.75

------------------------------------------------------------------------------------------------------------------------
-- small helper
local function clamp(x, a, b)
   if x < a then return a end
   if x > b then return b end
   return x
end

local function enterState(new_state)
   if (_state ~= new_state) then
      _state = new_state
      _state_time = 0.0
   end
end

-- Clamp a target position into a rectangular "leash" around base.
local function clampToLeash(target)
   local minx = _start_position_px:getX() - (LEASH_X_TILES * 24)
   local maxx = _start_position_px:getX() + (LEASH_X_TILES * 24)
   local miny = _start_position_px:getY() - (LEASH_Y_TILES * 24)
   local maxy = _start_position_px:getY() + (LEASH_Y_TILES * 24)
   return v2d.Vector2D(
      clamp(target:getX(), minx, maxx),
      clamp(target:getY(), miny, maxy)
   )
end

------------------------------------------------------------------------------------------------------------------------
function initialize()
   addSample("boom.wav")
   addHitbox(-18, -12, 36, 24)
   addShapeCircle(HIT_RADIUS, 0.0, 0.0)

   updateSpriteRect(0, 0, 0, _sprite_width, _sprite_height)
   setZ(30)
end

------------------------------------------------------------------------------------------------------------------------
-- Engine compatibility
function moveTo(x, y) end
function setPath(name, table) end

------------------------------------------------------------------------------------------------------------------------
function update(dt)
   _elapsed = _elapsed + dt
   _state_time = _state_time + dt

   -- dying / death
   if (not _dying) then
      if (_energy == 0) then
         _dying = true
         _death_time = _elapsed
         _sprite_offset_x = 0
         _sprite_offset_y = 2 * _sprite_height
         setActive(false)
         _position_at_death_px = _position_px
         setDamage(0)
      end
   end

   if (_dying) then
      setTransform(_position_at_death_px:getX(), _position_at_death_px:getY(), 0.0)
   else
      updateStateTransitions(dt)
      updateStateBehavior(dt)
      updateSpriteDirection()
      _position_px = _pos
   end

   if (_dead) then
      die()
   end

   updateAnimation(dt)
   checkForExplosion()
end

-- State transition logic
function updateStateTransitions(dt)
   -- Aggro is based on player distance to BASE (prevents dragging across map)
   local baseXDiffTiles = (_start_position_px:getX() // 24) - (_player_position_px:getX() // 24)
   local baseYDiffTiles = (_start_position_px:getY() // 24) - (_player_position_px:getY() // 24)
   local playerInAggro =
      (math.abs(baseXDiffTiles) <= AGGRO_X_TILES) and
      (math.abs(baseYDiffTiles) <= AGGRO_Y_TILES)

   -- If bat is outside leash already (dt spike / edge case), force return
   local outOfLeash =
      (math.abs((_pos:getX() // 24) - (_start_position_px:getX() // 24)) > LEASH_X_TILES) or
      (math.abs((_pos:getY() // 24) - (_start_position_px:getY() // 24)) > LEASH_Y_TILES)

   -- state transitions
   if (playerInAggro and (not outOfLeash)) then
      enterState(STATE_FOLLOW)
   else
      if (_state == STATE_FOLLOW) then
         enterState(STATE_RETURN)
      elseif (_state ~= STATE_RETURN) then
         enterState(STATE_IDLE)
      end
   end
end

-- State behavior logic
function updateStateBehavior(dt)
   if (_state == STATE_IDLE) then
      updateStateIdle(dt)
   elseif (_state == STATE_FOLLOW) then
      updateStateFollow(dt)
   elseif (_state == STATE_RETURN) then
      updateStateReturn(dt)
   end
end

-- Idle state behavior
function updateStateIdle(dt)
   local t = _elapsed
   local ox = math.sin(t * HOVER_OX_FREQ) * HOVER_OX_AMPL
   local oy = math.sin(t * HOVER_OY_FREQ) * HOVER_OY_AMPL

   local blend = clamp(_state_time / IDLE_HOVER_BLEND_TIME, 0.0, 1.0)

   local target = v2d.Vector2D(
      _start_position_px:getX() + ox * blend,
      _start_position_px:getY() + oy * blend
   )

   _pos = _pos:add(target:sub(_pos):mul(clamp(HOVER_EASE * dt, 0.0, 1.0)))
   _vel = _vel:mul(0.85)
   setTransform(_pos:getX(), _pos:getY(), 0.0)
end

-- Follow state behavior
function updateStateFollow(dt)
   local pv = _player_position_px:sub(_player_position_previous_px)
   local lead = pv:mul(FOLLOW_LEAD)

   -- base target: above player with slight lead
   local target = _player_position_px:add(lead)
   target = v2d.Vector2D(target:getX(), target:getY() + FOLLOW_OFFSET_Y)

   -- add orbit/fluter when very close so it doesn't "pin" to a single pixel
   -- compute closeness BEFORE leashing (more natural), then leash final target
   local toTarget = target:sub(_pos)
   local dist = toTarget:len()

   if (dist < ORBIT_START_DIST) then
      -- closeness factor: 0 at startDist, 1 at 0
      local k = clamp(1.0 - (dist / ORBIT_START_DIST), 0.0, 1.0) * ORBIT_MIX

      -- per-bat phase from initial _elapsed seed
      local phase = (_elapsed * ORBIT_FREQ) + (_attack_time or 0.0)
      local ox = math.cos(phase) * ORBIT_RADIUS
      local oy = math.sin(phase * 1.3) * (ORBIT_RADIUS * 0.65)

      target = v2d.Vector2D(target:getX() + ox * k, target:getY() + oy * k)
   end

   -- leash the target so bat never leaves its base zone
   target = clampToLeash(target)

   local desired = target:sub(_pos):norm():mul(FOLLOW_SPEED)
   _vel = _vel:add(desired:sub(_vel):mul(clamp(FOLLOW_TURN * dt, 0.0, 1.0)))
   _pos = _pos:add(_vel:mul(dt))
   setTransform(_pos:getX(), _pos:getY(), 0.0)
end

-- Return state behavior
function updateStateReturn(dt)
   local home = v2d.Vector2D(_start_position_px:getX(), _start_position_px:getY())
   local toHome = home:sub(_pos)
   local dist = toHome:len()

   if (dist <= RETURN_DONE_DIST) then
      _pos = home
      _vel = v2d.Vector2D(0, 0)
      enterState(STATE_IDLE)
      _state_time = 0.0
   else
      local slow = clamp(dist / RETURN_SLOW_RADIUS, 0.15, 1.0)
      local desired = toHome:norm():mul(RETURN_SPEED * slow)

      _vel = _vel:add(desired:sub(_vel):mul(clamp(RETURN_TURN * dt, 0.0, 1.0)))
      _pos = _pos:add(_vel:mul(dt))
   end

   setTransform(_pos:getX(), _pos:getY(), 0.0)
end

-- Update sprite direction based on player position
function updateSpriteDirection()
   local bx = _pos:getX()
   local px = _player_position_px:getX()
   if (_can_explode) then
      _sprite_offset_y = (px > bx) and (4 * _sprite_height) or (5 * _sprite_height)
   else
      _sprite_offset_y = (px > bx) and 0 or _sprite_height
   end
end

-- Update animation and sprite
function updateAnimation(dt)
   local sprite_index = 0
   if (_dying) then
      sprite_index = math.min(math.floor((_elapsed - _death_time) * ANIMATION_SPEED_DEATH), 21)
      if (sprite_index == 21) then
         sprite_index = 20
         _dead = true
      end
   else
      sprite_index = math.floor(math.fmod(_elapsed * ANIMATION_SPEED_IDLE, ATTACK_SPRITE_COUNT))
   end

   local update_sprite = false
   if (sprite_index ~= _sprite_offset_x) then
      _sprite_offset_x = sprite_index
      update_sprite = true
   end

   if (update_sprite) then
      updateSpriteRect(
         0,
         _sprite_offset_x * _sprite_width,
         _sprite_offset_y,
         _sprite_width,
         _sprite_height
      )
   end
end

-- Check for explosion conditions
function checkForExplosion()
   if (_can_explode and not _exploded and not _dead and not _dying) then
      local intersects = intersectsWithPlayer(_position_px:getX() - 12, _position_px:getY() + 12, 24, 24)
      if (intersects) then
         _exploded = true
         _dead = true

         boom(0.0, 1.0, 0.5)
         playSample("boom.wav")
         playDetonationAnimation(_position_px:getX(), _position_px:getY())
         damage(10, 0.0, 0.0)
      end
   end
end

------------------------------------------------------------------------------------------------------------------------
function retrieveProperties()
   updateProperties(properties)
end

------------------------------------------------------------------------------------------------------------------------
function setStartPosition(x, y)
   _start_position_px = v2d.Vector2D(x, y)
   _pos = v2d.Vector2D(x, y)
   _vel = v2d.Vector2D(0, 0)
   _position_px = _pos
   _state = STATE_IDLE
   _state_time = 0.0
end

------------------------------------------------------------------------------------------------------------------------
function movedTo(x, y)
   _position_px = v2d.Vector2D(x, y)

   if (_start_position_px:getX() == 0 and _start_position_px:getY() == 0) then
      _start_position_px = v2d.Vector2D(x, y)
   end

   _pos = v2d.Vector2D(x, y)
end

------------------------------------------------------------------------------------------------------------------------
function playerMovedTo(x, y)
   _player_position_previous_px = _player_position_px
   _player_position_px = v2d.Vector2D(x, y)
end

------------------------------------------------------------------------------------------------------------------------
function hit(damage_value)
   _energy = _energy - 1
end

------------------------------------------------------------------------------------------------------------------------
function writeProperty(key, value)
   if (key == "exploding") then
      _can_explode = true
      _sprite_offset_y = 4 * 3 * 24
   elseif (key == "audio_update_behavior") then
      local update_behavior = audioUpdateBehaviorFromString(value)
      setAudioUpdateBehavior(update_behavior)
   end
end
