require "data/scripts/enemies/constants"
v2d = require "data/scripts/enemies/vectorial2"

-- arrow rows:
--    points left:  5
--    points right: 6
--    points down:  7
--    points up:    8
--
-- start animation:
--    column 0..8
--
-- arrow fly
--    column 9..12
--
-- arrow rotate
--    column 13..18
--
-- arrow fade out
--    column 19..23


------------------------------------------------------------------------------------------------------------------------
properties = {
   staticBody = true,
   sprite = "data/sprites/enemy_arrowtrap_2.png",
   damage = 0
}


------------------------------------------------------------------------------------------------------------------------
_start_fire_timer = 1

_fire_interval = 3.0
_fire_delays = {}
_fire_delay_elapsed = {}
_sprite_indices = {}
_fired = {false, false, false}

_position = v2d.Vector2D(0, 0)
_player_position = v2d.Vector2D(0, 0)
_elapsed = 0.0
_fire_direction = v2d.Vector2D(0, 0)
_fire_offset = {v2d.Vector2D(0, 0), v2d.Vector2D(0, 0), v2d.Vector2D(0, 0)}
_speed = 1.5
_offset = v2d.Vector2D(0, 0)
_muzzle_width = 0
_muzzle_height = 0
_muzzle_offset = v2d.Vector2D(0, 0)

SPRITE_WIDTH = 3 * 24
SPRITE_HEIGHT = 3 * 24


-- physical box is just 24 * 24px
-- pixel box is 3 * 24 * 3 * 24px
--
--   +---+---+---+
--   |   |   |   |
--   +---+---+---+
--   |   |///|   |
--   +---+---+---+
--   |   |   |   |
--   +---+---+---+
--


------------------------------------------------------------------------------------------------------------------------
function initialize()

   _fire_delays = {1.0, 1.25, 1.5}
   _sprite_indices = {0, 0, 0}
   _fire_delay_elapsed = {false, false, false}

   -- add sprites 1, 2, 3 for each muzzle
   addSprite()
   addSprite()
   addSprite()

   addShapeRect(0.25, 0.25, 0.25, 0.25)
   addSample("boom.wav")
   addWeapon(WeaponType["Bow"], 50, 60, 0.1) -- interval, damage, radius
   updateAlignment(Alignment["AlignmentRight"])
   setSpriteOffset(0, 12, 12)

end


------------------------------------------------------------------------------------------------------------------------
function updateAlignment(alignment)

   offset = v2d.Vector2D(0, 0)

   -- rows in spriteset
   -- 6: left
   -- 7: right
   -- 8: down
   -- 9: up

   if (alignment == Alignment["AlignmentUp"]) then
      offset:setX(5)
      offset:setY(1)
      _fire_direction = v2d.Vector2D(0.0, -1.0)
      _fire_offset[1] = v2d.Vector2D(3.0, -16.0)
      _fire_offset[2] = v2d.Vector2D(11.0, -16.0)
      _fire_offset[3] = v2d.Vector2D(19.0, -16.0)
      _offset = v2d.Vector2D(0, 9 * 24)
      _muzzle_width = 8
      _muzzle_height = 24
      _muzzle_offset:setX(4)
      _muzzle_offset:setY(-12)
      setSpriteOffset(1, _muzzle_offset:getX(),                     _muzzle_offset:getY())
      setSpriteOffset(2, _muzzle_offset:getX() + _muzzle_width,     _muzzle_offset:getY())
      setSpriteOffset(3, _muzzle_offset:getX() + _muzzle_width * 2, _muzzle_offset:getY())
   elseif (alignment == Alignment["AlignmentDown"]) then
      offset:setX(5)
      offset:setY(0)
      _fire_direction = v2d.Vector2D(0.0, 1.0)
      _fire_offset[1] = v2d.Vector2D(4.0, 40.0)
      _fire_offset[2] = v2d.Vector2D(12.0, 40.0)
      _fire_offset[3] = v2d.Vector2D(20.0, 40.0)
      _offset = v2d.Vector2D(0, 8 * 24)
      _muzzle_width = 8
      _muzzle_height = 24
      _muzzle_offset:setX(4)
      _muzzle_offset:setY(36)
      setSpriteOffset(1, _muzzle_offset:getX(),                     _muzzle_offset:getY())
      setSpriteOffset(2, _muzzle_offset:getX() + _muzzle_width,     _muzzle_offset:getY())
      setSpriteOffset(3, _muzzle_offset:getX() + _muzzle_width * 2, _muzzle_offset:getY())
   elseif (alignment == Alignment["AlignmentLeft"]) then
      offset:setX(0)
      offset:setY(0)
      _fire_direction = v2d.Vector2D(-1.0, 0.0)
      _fire_offset[1] = v2d.Vector2D(-16.0, 3.0)
      _fire_offset[2] = v2d.Vector2D(-16.0, 11.0)
      _fire_offset[3] = v2d.Vector2D(-16.0, 19.0)
      _offset = v2d.Vector2D(0, 6 * 24)
      _muzzle_width = 24
      _muzzle_height = 8
      _muzzle_offset:setX(-12)
      _muzzle_offset:setY(4)
      setSpriteOffset(1, _muzzle_offset:getX(), _muzzle_offset:getY())
      setSpriteOffset(2, _muzzle_offset:getX(), _muzzle_offset:getY() + _muzzle_height)
      setSpriteOffset(3, _muzzle_offset:getX(), _muzzle_offset:getY() + _muzzle_height * 2)
   elseif (alignment == Alignment["AlignmentRight"]) then
      offset:setX(0)
      offset:setY(1)
      _fire_direction = v2d.Vector2D(1.0, 0.0)
      _fire_offset[1] = v2d.Vector2D(36.0, 3.0)
      _fire_offset[2] = v2d.Vector2D(36.0, 11.0)
      _fire_offset[3] = v2d.Vector2D(36.0, 19.0)
      _offset = v2d.Vector2D(0, 7 * 24)
      _muzzle_width = 24
      _muzzle_height = 8
      _muzzle_offset:setX(36)
      _muzzle_offset:setY(4)
      setSpriteOffset(1, _muzzle_offset:getX(), _muzzle_offset:getY())
      setSpriteOffset(2, _muzzle_offset:getX(), _muzzle_offset:getY() + _muzzle_height)
      setSpriteOffset(3, _muzzle_offset:getX(), _muzzle_offset:getY() + _muzzle_height * 2)
   end

   updateSpriteRect(
      0,
      offset:getX() * SPRITE_WIDTH,
      offset:getY() * SPRITE_HEIGHT,
      SPRITE_WIDTH,
      SPRITE_HEIGHT
   )

   for i = 1, 3, 1
   do
      updateSpriteRect(
         i,
         _offset.x,
         _offset.y,
         _muzzle_width,
         _muzzle_height
      )

   end
end


------------------------------------------------------------------------------------------------------------------------
function writeProperty(key, value)

   -- print(string.format("write property: %s %s", key, value))

   if (key == "alignment") then

      alignment = Alignment["AlignmentRight"]

      if (value == "right") then
         alignment = Alignment["AlignmentRight"]
      elseif (value == "left") then
         alignment = Alignment["AlignmentLeft"]
      elseif (value == "up") then
         alignment = Alignment["AlignmentUp"]
      elseif (value == "down") then
         alignment = Alignment["AlignmentDown"]
      end

      updateAlignment(alignment)
   elseif (key == "time_offset_s") then
      _elapsed = _elapsed + value
   end
end


------------------------------------------------------------------------------------------------------------------------
function timeout(id)
   -- print(string.format("timeout: %d", id))

   if (id == _start_fire_timer) then
      -- mReadytoFire = true
   end

end


------------------------------------------------------------------------------------------------------------------------
function fire(muzzle_index)
   useGun(
      0,
      _position:getX() + _fire_offset[muzzle_index]:getX(),
      _position:getY() + _fire_offset[muzzle_index]:getY(),
      _fire_direction:getX() * _speed,
      _fire_direction:getY() * _speed
   );
end


------------------------------------------------------------------------------------------------------------------------
function retrieveProperties()
   updateProperties(properties)
end


------------------------------------------------------------------------------------------------------------------------
function movedTo(x, y)
   -- print(string.format("moved to: %f, %f", x, y))
   _position = v2d.Vector2D(x, y)
end


------------------------------------------------------------------------------------------------------------------------
function playerMovedTo(x, y)
   -- print(string.format("player moved to: %f, %f", x, y))
   _player_position = v2d.Vector2D(x, y)
end


------------------------------------------------------------------------------------------------------------------------
function update(dt)

   _elapsed = _elapsed + dt

   if (_elapsed > _fire_interval) then
      _elapsed = 0.0
      _fire_delay_elapsed = {false, false, false}
      _sprite_indices = {0, 0, 0}
      _fired = {false, false, false}
   end

   for i = 1, 3, 1
   do
      if (not _fire_delay_elapsed[i]) then
         if (_elapsed > _fire_delays[i]) then
            _fire_delay_elapsed[i]=true
         end
      end
   end

   -- init all muzzle sprites with idle (empty texture)
   sprite_indices_current = {9, 9, 9}

   for i = 1, 3, 1
   do
      if (_fire_delay_elapsed[i]) then
         index = math.floor((_elapsed - _fire_delays[i]) * 20.0)

         -- fire actual arrow when the animation has been fully played
         if (index > 8) then
            if (not _fired[i]) then
               _fired[i]=true
               fire(i)
               -- print(string.format("fire: %d", i))
            end
         end

         -- clamp sprite index at 9
         if (index > 9) then
            index = 9
         end

         sprite_indices_current[i] = index
      else
         -- load arrow in arrow trap
         index = math.floor(_elapsed * 20.0)
         -- clamp sprite index at 5
         if (index > 5) then
            index = 5
         end
         sprite_indices_current[i] = index
         -- print(string.format("update: %d: %d", i, index))
      end
   end

   -- update each muzzle texture
   for i = 1, 3, 1
   do
      if (sprite_indices_current[i] ~= _sprite_indices[i]) then
         updateSpriteRect(
            i,
            _offset:getX() + sprite_indices_current[i] * 24,
            _offset:getY(),
            _muzzle_width,
            _muzzle_height
         )
      end
   end
end

