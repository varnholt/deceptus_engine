require "data/scripts/enemies/constants"
v2d = require "data/scripts/enemies/vectorial2"


------------------------------------------------------------------------------------------------------------------------
properties = {
   static_body = true,
   sprite = "data/sprites/enemy_bonefish.png",
   damage = 4
}


------------------------------------------------------------------------------------------------------------------------
_position = v2d.Vector2D(0, 0)
_speed = 1.0
_frequency = 1.0
_amplitude = 1.0
_player_position = v2d.Vector2D(0, 0)
_center = v2d.Vector2D(0, 0)
_distance = 0
_elapsed = math.random(0, 3)
_sprite_index = 0
_direction_x = 1.0  -- X component of movement direction (normalized)
_direction_y = 0.0  -- Y component of movement direction (normalized)
_prev_offset = 0.0  -- Previous sine offset for sprite flip detection
_points_left = false  -- For horizontal movement direction


------------------------------------------------------------------------------------------------------------------------
function initialize()

   patrol_path = {}
   patrol_epsilon = 1.0

   addShapeRect(0.3, 0.15, 0, 0.05)
   updateSpriteRect(0, 0, 0, 72, 48)
end


------------------------------------------------------------------------------------------------------------------------
function update(dt)
   y_offset = 0
   update_sprite = false

   -- get sprite index
   _elapsed = _elapsed + dt
   sprite_index = math.floor(math.fmod(_elapsed * 20.0, 30))

   -- Calculate position using direction factors (no branching needed)
   local offset = 0.5 * math.sin(_elapsed * _speed) * _distance
   local pos_x = _center:getX() + offset * _direction_x
   local pos_y = _center:getY() + offset * _direction_y

   -- Add perpendicular oscillation (direction rotated 90 degrees)
   local perp_x = -_direction_y
   local perp_y = _direction_x
   local perp_offset = math.sin(_elapsed * _frequency) * _amplitude
   pos_x = pos_x + perp_x * perp_offset
   pos_y = pos_y + perp_y * perp_offset

   -- Detect movement direction for sprite flip (only when moving primarily horizontally)
   if math.abs(_direction_x) > math.abs(_direction_y) then
      if offset > _prev_offset then
         _points_left = false  -- Moving right
      else
         _points_left = true   -- Moving left
      end
   end
   _prev_offset = offset

   -- update transform
   setTransform(pos_x, pos_y, 0.0)

   -- update sprite index
   if (sprite_index ~= _sprite_index) then
      _sprite_index = sprite_index
      update_sprite = true
   end

   -- Handle sprite flipping based on movement direction
   if math.abs(_direction_x) > math.abs(_direction_y) and _points_left then
      y_offset = 0   -- Normal when moving left
   else
      y_offset = 48  -- Flip when moving right (or vertical movement)
   end

   if (update_sprite) then
      updateSpriteRect(0, _sprite_index * 72, y_offset, 72, 48) -- x, y, width, height
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
   -- print(string.format("Received %d arguments:", #table))

   local i = 0
   local x = 0.0;
   local y = 0.0;
   local v = {}

   for key, value in pairs(table) do

      if ((i % 2) == 0) then
         x = value
      else
         y = value
         -- print(string.format("v%d: %f, %f", (i - 1) / 2, x, y))
         v[(i - 1) / 2] = v2d.Vector2D(x, y)
      end

      i = i + 1
   end

   if (name == "path") then
      patrol_path = v

      -- compute movement direction as normalized factors
      local left_arr = patrol_path[0]
      local left = v2d.Vector2D(left_arr:getX(), left_arr:getY())
      local right_arr = patrol_path[1]
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
      end
   end
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
   _player_position = v2d.Vector2D(x, y)
end


------------------------------------------------------------------------------------------------------------------------
function timeout(id)
end
