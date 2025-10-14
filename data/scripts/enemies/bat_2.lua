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
_position_px = v2d.Vector2D(0, 0)
_position_at_death_px = v2d.Vector2D(0, 0)
_player_position_px = v2d.Vector2D(0, 0)
_player_position_previous_px = v2d.Vector2D(0, 0)
_elapsed = math.random(0, 3)
_attack_time = 0
_idle_time = 0
_activated = false
_move_range_y_px = 48
_sprite_offset_x = 0
_sprite_offset_y = 0 -- 9 * 24
_sprite_width = 72
_sprite_height = 72
_start_position_px = v2d.Vector2D(0, 0)
_dying = false
_dead = false
_death_time = 0
_transform_y = 0
_attack = false
_can_explode = false
_exploded = false
_path = {}
_energy = 3
ANIMATION_SPEED = 40.0
ANIMATION_SPEED_IDLE = 20.0
ANIMATION_SPEED_DEATH = 20.0
HIT_RADIUS = 0.3
ATTACK_DURATION = 2.0
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

   _attack_time = _elapsed

   bx = _position_px:getX()
   by = _position_px:getY()

   px = _player_position_px:getX()
   py = _player_position_px:getY()

   if (_can_explode) then
      _sprite_offset_y = (px > bx) and (4 * _sprite_height) or (5 * _sprite_height)
   else
      _sprite_offset_y = (px > bx) and 0 or _sprite_height
   end

   sx = _start_position_px:getX()
   sy = _start_position_px:getY()

   k1 = Key:create{x = bx, y = by, time = 0.0}
   k2 = Key:create{x = px, y = py, time = ATTACK_DURATION / 2.0} -- player pos
   k3 = Key:create{x = sx, y = sy, time = ATTACK_DURATION}       -- go back

   _path = {k1, k2, k3}
end


------------------------------------------------------------------------------------------------------------------------
function moveTo(x, y)
end


------------------------------------------------------------------------------------------------------------------------
function update(dt)

   update_sprite = false
   _elapsed = _elapsed + dt

   -- check if dead
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
      -- stickytape the bat to its death location
      setTransform(_position_at_death_px:getX(), _position_at_death_px:getY(), 0.0)
   else
      -- consider attacking
      idle = _elapsed < _idle_time
      if (not _attack or idle) then

         -- make sure bat is not too far away from player in x
         xDiff = _position_px:getX() // 24 - _player_position_px:getX() // 24
         if (math.abs(xDiff) < 6) then

            -- make sure bat is not too far away from player in y
            yDiff = _position_px:getY() // 24 - _player_position_px:getY() // 24
            if (yDiff < 0 and yDiff > -7 and not idle) then

               shoot()
            end
         end

         -- update transform
         -- _transform_y = 0.25 * math.sin(_elapsed) * _move_range_y_px
         _transform_y = 0
         setTransform(_start_position_px:getX(), _start_position_px:getY() + _transform_y, 0.0)

      -- carry out attack
      else
         time = (_elapsed - _attack_time) -- / ATTACK_DURATION
         p = getValueCubic(_path, time)

         -- print(string.format("i: %f, x: %f, y: %f", time, p:getX(), p:getY()))
         setTransform(p:getX(), p:getY(), 0.0)

         if (time > ATTACK_DURATION) then
            _attack = false
            _idle_time = _elapsed + ATTACK_DURATION
         end
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
      update_sprite = true
   end

   -- updateDebugRect(0, _position_px:getX() - 12, _position_px:getY() + 12, 24, 24)

   if (_can_explode and not _exploded and not _dead) then

      -- |24px|
      -- +----+----+----+ - - -
      -- |    |    |    | 24px
      -- +----+----+----+ - - -
      -- |    |    |    |
      -- +----+----+----+
      -- |    |(XX)|    |
      -- +----+----+----+

      intersects = intersectsWithPlayer(_position_px:getX() - 12, _position_px:getY() + 12, 24, 24)

      if (intersects) then

         _exploded = true
         _dead = true

         boom(0.0, 1.0, 0.5)
         playSample("boom.wav")
         playDetonationAnimation(_position_px:getX(), _position_px:getY())
         damage(10, 0.0, 0.0)
      end
   end

   if (update_sprite) then
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
   _start_position_px = v2d.Vector2D(x, y)
end


------------------------------------------------------------------------------------------------------------------------
function movedTo(x, y)
   _position_px = v2d.Vector2D(x, y)
end


------------------------------------------------------------------------------------------------------------------------
function playerMovedTo(x, y)
   _player_position_px = v2d.Vector2D(x, y)
end


------------------------------------------------------------------------------------------------------------------------
function hit(damage_value)
  _energy = _energy - 1
end


------------------------------------------------------------------------------------------------------------------------
function writeProperty(key, value)

   -- print(string.format("write property: %s %s", key, value))

   if (key == "exploding") then
      _can_explode = true
      _sprite_offset_y = 4 * 3 * 24
   elseif (key == "audio_update_behavior") then
      update_behavior = audioUpdateBehaviorFromString(value)
      setAudioUpdateBehavior(update_behavior)
   end
end
