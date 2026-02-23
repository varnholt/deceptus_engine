require "data/scripts/enemies/constants"
v2d = require "data/scripts/enemies/vectorial2"


------------------------------------------------------------------------------------------------------------------------
properties = {
   static_body = true,
   sprite = "data/sprites/enemy_bonefish.png",
   damage = 4
}


------------------------------------------------------------------------------------------------------------------------
-- constants
local SPRITE_WIDTH = 72
local SPRITE_HEIGHT = 48
local ANIMATION_SPEED = 20.0
local ANIMATION_FRAMES = 30
local FLIP_OFFSET = 48


------------------------------------------------------------------------------------------------------------------------
-- state
_speed = 1.0
_frequency = 1.0
_amplitude = 1.0
_center = v2d.Vector2D(0, 0)
_distance = 0
_elapsed = math.random(0, 3)
_sprite_index = 0
_direction_x = 1.0  -- x component of movement direction (normalized)
_direction_y = 0.0  -- y component of movement direction (normalized)
_prev_offset = 0.0  -- previous sine offset for sprite flip detection
_horizontal_movement = false
_points_left = false


------------------------------------------------------------------------------------------------------------------------
function initialize()
   addShapeRect(0.3, 0.15, 0, 0.05)
   updateSpriteRect(0, 0, 0, SPRITE_WIDTH, SPRITE_HEIGHT)
end


------------------------------------------------------------------------------------------------------------------------
function update(dt)
   y_offset = 0
   update_sprite = false

   -- get sprite index
   _elapsed = _elapsed + dt
   sprite_index = math.floor(math.fmod(_elapsed * ANIMATION_SPEED, ANIMATION_FRAMES))

   -- calculate position using direction factors (no branching needed)
   local offset = 0.5 * math.sin(_elapsed * _speed) * _distance
   local pos_x = _center:getX() + offset * _direction_x
   local pos_y = _center:getY() + offset * _direction_y

   -- add perpendicular oscillation (direction rotated 90 degrees)
   local perp_x = -_direction_y
   local perp_y = _direction_x
   local perp_offset = math.sin(_elapsed * _frequency) * _amplitude
   pos_x = pos_x + perp_x * perp_offset
   pos_y = pos_y + perp_y * perp_offset

   -- detect movement direction for sprite flip (only when moving primarily horizontally)
   local offset_delta = offset - _prev_offset
   _points_left = (offset_delta * _direction_x) < 0.0
   _prev_offset = offset

   -- update transform
   setTransform(pos_x, pos_y, 0.0)

   -- update sprite index
   if (sprite_index ~= _sprite_index) then
      _sprite_index = sprite_index
      update_sprite = true
   end

   -- handle sprite flipping based on movement direction
   y_offset = (_horizontal_movement and _points_left) and 0 or FLIP_OFFSET

   if (update_sprite) then
      updateSpriteRect(0, _sprite_index * SPRITE_WIDTH, y_offset, SPRITE_WIDTH, SPRITE_HEIGHT)
   end

end


------------------------------------------------------------------------------------------------------------------------
function writeProperty(parameter, value)

   if (parameter == "speed") then
      _speed = value
   elseif (parameter == "frequency") then
      _frequency = value
   elseif (parameter == "amplitude") then
      _amplitude = value
   end
end

------------------------------------------------------------------------------------------------------------------------
function setPath(name, table)
   -- print(string.format("received %d arguments:", #table))

   local v = {}
   local num_points = 0

   for i = 1, #table, 2 do
      local x = table[i]
      local y = table[i + 1]
      -- print(string.format("v%d: %f, %f", num_points, x, y))
      v[num_points] = v2d.Vector2D(x, y)
      num_points = num_points + 1
   end

   if (name == "path") then
      -- compute movement direction as normalized factors
      local left_arr = v[0]
      local left = v2d.Vector2D(left_arr:getX(), left_arr:getY())
      local right_arr = v[1]
      local right = v2d.Vector2D(right_arr:getX(), right_arr:getY())
      local dx = right:getX() - left:getX()
      local dy = right:getY() - left:getY()

      -- calculate center point
      _center = v2d.Vector2D((left:getX() + right:getX()) / 2.0, (left:getY() + right:getY()) / 2.0)

      -- calculate distance and normalized direction
      _distance = math.sqrt(dx * dx + dy * dy)
      if _distance > 0 then
         _direction_x = dx / _distance
         _direction_y = dy / _distance
         _horizontal_movement = math.abs(_direction_x) > math.abs(_direction_y)
      end
   end
end


------------------------------------------------------------------------------------------------------------------------
function retrieveProperties()
   updateProperties(properties)
end


------------------------------------------------------------------------------------------------------------------------
function movedTo(x, y)
end


------------------------------------------------------------------------------------------------------------------------
function playerMovedTo(x, y)
end


------------------------------------------------------------------------------------------------------------------------
function timeout(id)
end
