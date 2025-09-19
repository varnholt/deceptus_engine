------------------------------------------------------------------------------------------------------------------------
require "data/scripts/enemies/constants"
require "data/scripts/enemies/interpolation"
v2d = require "data/scripts/enemies/vectorial2"


--  0  .x...............................
--  1  .x...............................
--  2  .................................
--  3  .................................
--  4  .................................
--  5  .................................
--  6  .................................
--  7  .................................
--  8  .................................
--  9  000111222333444555666777888wwwbbb
-- 10  000111222333444555666777888wwwbbb
--
--     012345678901234567890123456789012
--     0  1  2  3  4  5  6  7  8  9  0

-- 221, 77
-- 210, 78
-- 223, 77
-- player: 206, 82

------------------------------------------------------------------------------------------------------------------------
-- interpolation keys
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
_position = v2d.Vector2D(0, 0)
_positionAtDeath = v2d.Vector2D(0, 0)
_player_position = v2d.Vector2D(0, 0)
_player_positionPrevious = v2d.Vector2D(0, 0)
_elapsed = math.random(0, 3)
_attackTime = 0
_idle_time = 0
_activated = false
_move_range_y = 48
_sprite_offset_x = 0
_sprite_offset_y = 0 -- 9 * 24
_sprite_width = 72
_sprite_height = 72
_start_position = v2d.Vector2D(0, 0)
_dying = false
_dead = false
_death_time = 0
_transform_x = 0
_transform_y = 0
_attack = false
_can_explode = false
_exploded = false
_path = {}
_energy = 3
_speed = 0.1

ANIMATION_SPEED = 40.0
ANIMATION_SPEED_IDLE = 20.0
ANIMATION_SPEED_DEATH = 20.0
HIT_RADIUS = 0.3
ATTACK_DURATION = 1.0
ATTACK_SPRITE_COUNT = 9


------------------------------------------------------------------------------------------------------------------------
function initialize()
   addSample("boom.wav")
   addHitbox(-18, -12, 36, 24)
   addShapeCircle(HIT_RADIUS, 0.0, 0.0)
   updateSpriteRect(
      0,
      0,
      0,
      _sprite_width,
      _sprite_height
   )
   setZ(30) -- somewhere in the foreground

   -- addDebugRect()
end


------------------------------------------------------------------------------------------------------------------------
function shoot()
   _attack = true

   _attackTime = _elapsed

   bx = _position:getX()
   by = _position:getY()

   px = _player_position:getX()
   py = _player_position:getY()

   if (_can_explode) then
      _sprite_offset_y = (px > bx) and (4 * _sprite_height) or (5 * _sprite_height)
   else
      _sprite_offset_y = (px > bx) and 0 or _sprite_height
   end

   sx = _start_position:getX()
   sy = _start_position:getY()

   k1 = Key:create{x = bx, y = by, time = 0.0}
   k2 = Key:create{x = px, y = py, time = 0.5} -- player pos
   k3 = Key:create{x = sx, y = sy, time = 1.0} -- go back

   _path = {k1, k2, k3}
end


------------------------------------------------------------------------------------------------------------------------
function moveTo(x, y)
end


------------------------------------------------------------------------------------------------------------------------
function update(dt)

   updateSprite = false
   _elapsed = _elapsed + dt

   -- check if dead
   if (not _dying) then
      if (_energy == 0) then
         _dying = true
         _death_time = _elapsed
         _sprite_offset_x = 0
         _sprite_offset_y = 2 * _sprite_height
         setActive(false)
         _positionAtDeath = _position
      end
   end

   if (_dying) then
      -- stickytape the bat to its death location
      setTransform(_positionAtDeath:getX(), _positionAtDeath:getY(), 0.0)
   else
      -- consider attacking
      idle = _elapsed < _idle_time

      if (not _attack or idle) then

         -- update transform
         -- _transform_y = 0.25 * math.sin(_elapsed) * _move_range_y
         _transform_y = 0

         if (_activated) then
            _transform_x = _transform_x + dt * _speed
         end
         setTransform(_start_position:getX() + _transform_x, _start_position:getY() + _transform_y, 0.0)
      end
   end

   if (_dead) then
      die()
   end

   -- update sprite offset
   if (_dying) then
      -- 12 sprites per row
      sprite_index = math.min(math.floor((_elapsed - _death_time) * ANIMATION_SPEED_DEATH), 21)
      if (sprite_index == 21) then
         sprite_index = 20
         _dead = true
      end
   elseif (idle or not _attack) then
      sprite_index = math.floor(math.fmod(_elapsed * ANIMATION_SPEED_IDLE, ATTACK_SPRITE_COUNT))
   else
      sprite_index = math.floor(math.fmod(_elapsed * ANIMATION_SPEED, ATTACK_SPRITE_COUNT))
   end

   -- update sprite index
   if (index ~= _sprite_offset_x) then
      _sprite_offset_x = sprite_index
      updateSprite = true
   end

   -- updateDebugRect(0, _position:getX() - 12, _position:getY() + 12, 24, 24)

   if (_activated and _can_explode and not _exploded and not _dead) then

      -- |24px|
      -- +----+----+----+ - - -
      -- |    |    |    | 24px
      -- +----+----+----+ - - -
      -- |    |    |    |
      -- +----+----+----+
      -- |    |(XX)|    |
      -- +----+----+----+

      intersects = intersectsWithPlayer(_position:getX() - 12, _position:getY() + 12, 24, 24)

      if (intersects) then

         _exploded = true
         _dead = true

         boom(0.0, 1.0, 0.5)
         playSample("boom.wav")
         playDetonationAnimation(_position:getX(), _position:getY())
         damage(10, 0.0, 0.0)
      end
   end

   if (updateSprite) then
      updateSpriteRect(
         0,
         _sprite_offset_x * _sprite_width,
         _sprite_offset_y,
         _sprite_width,
         _sprite_height
      ) -- x, y, width, height
   end

end


------------------------------------------------------------------------------------------------------------------------
function setPath(name, table)
end


------------------------------------------------------------------------------------------------------------------------
function retrieveProperties()
   updateProperties(properties)
end


------------------------------------------------------------------------------------------------------------------------
function setStartPosition(x, y)
   _start_position = v2d.Vector2D(x, y)
end


------------------------------------------------------------------------------------------------------------------------
function movedTo(x, y)
   _position = v2d.Vector2D(x, y)
end


------------------------------------------------------------------------------------------------------------------------
function playerMovedTo(x, y)
   _player_position = v2d.Vector2D(x, y)
end


------------------------------------------------------------------------------------------------------------------------
function hit(damage_value)
  _energy = _energy - 1
end


------------------------------------------------------------------------------------------------------------------------
function writeProperty(key, value)

   print(string.format("write property: %s %s", key, value))

   if (key == "activated") then
      _activated = value == "true" and true or false
   elseif (key == "speed") then
      _speed = tonumber(value)
   elseif (key == "exploding") then
      _can_explode = true
      _sprite_offset_y = 4 * 3 * 24
   elseif (key == "audio_update_behavior") then
      update_behavior = audioUpdateBehaviorFromString(value)
      setAudioUpdateBehavior(update_behavior)
   end
end
