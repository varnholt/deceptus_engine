-- row 0: idle left
-- row 1: idle right
-- row 2: walk left
-- row 3: walk right
-- row 4: hide left
-- row 5: hide right
-- row 6, 7: idle left
-- row 8, 9: idle right
--
-- hide left: 48-59
-- hide right: 60-71
-- reveal left: 51-48
-- reveal right: 63-60

-- 12 sprites per row

-- size is 2 x 24, 2 x 24
-- origin is in the bottom center
-- +---+---+
-- | ##|## |
-- +---+---+
-- | ##|## |
-- +---+---+

require "data/scripts/enemies/constants"
require "data/scripts/enemies/helpers"
v2d = require "data/scripts/enemies/vectorial2"

-- enemy configuration
properties = {
   sprite = "data/sprites/enemy_spiky.png",
   velocity_walk_max = 1.0,
   acceleration_ground = 0.1,
   damage = 3
}

CYCLE_IDLE = 1
CYCLE_WALK = 2
CYCLE_HIDE = 3
CYCLE_HIDE_IDLE = 4
CYCLE_APPEAR = 5
CYCLE_LENGTHS =  {12, 12, 12, 15, 4}
ANIMATION_SPEEDS = {5.0, 5.0, 5.0, 5.0, 5.0}
SPRITE_SIZE = 48

------------------------------------------------------------------------------------------------------------------------
_keys_pressed = 0
_position = v2d.Vector2D(0, 0)
_player_position = v2d.Vector2D(0, 0)
_points_to_left = false
_patrol_time = 0.0
_current_sprite_elapsed = 99.0
_animation_flag_inverse = false
_patrol_path = {}
_patrol_index = 1
_patrol_epsilon = 1.0
_patrol_path_arrived = false
_current_cycle = CYCLE_WALK
_elapsed = 0.0
_idle = false


------------------------------------------------------------------------------------------------------------------------
function initialize()
   addShapeCircle(0.24, 0.0, 0.24)
   updateSpriteRect(0, 0, 0, 48, 48)
end


------------------------------------------------------------------------------------------------------------------------
function retrieveProperties()
   updateProperties(properties)
end


------------------------------------------------------------------------------------------------------------------------
function keyPressed(key)
   _keys_pressed = (_keys_pressed | key)
end


------------------------------------------------------------------------------------------------------------------------
function keyReleased(key)
   _keys_pressed = _keys_pressed & (~key)
end


------------------------------------------------------------------------------------------------------------------------
function goLeft()
   _points_to_left = true
   _patrol_path_arrived = false
   keyReleased(Key["KeyRight"])
   keyPressed(Key["KeyLeft"])
end


------------------------------------------------------------------------------------------------------------------------
function goRight()
   _points_left = false
   _patrol_path_arrived = false
   keyReleased(Key["KeyLeft"])
   keyPressed(Key["KeyRight"])
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
-- |          o      p          | x
-- |          p      o          | x
function followPlayer()
   local epsilon = 5
   if (_player_position:getX() > _position:getX() + epsilon) then
      goRight()
   elseif (_player_position:getX() < _position:getX() - epsilon) then
      goLeft()
   else
      _keys_pressed = 0
   end
end


------------------------------------------------------------------------------------------------------------------------
function updateSprite(dt)
   _current_sprite_elapsed = _current_sprite_elapsed + dt * ANIMATION_SPEEDS[_current_cycle]
   sprite_index = math.floor(math.fmod(_current_sprite_elapsed, getMaxCycle()))

   if (_current_sprite ~= sprite_index) then
      _current_sprite = sprite_index

      -- appear is an inverse of hide
      if (next_animation_flag_inverse) then
         sprite_index = getMaxCycle() - sprite_index - 1
      end

      updateSpriteRect(
         0,
         sprite_index * SPRITE_SIZE,
         getSpriteOffsetY(),
         SPRITE_SIZE,
         SPRITE_SIZE
      )
   end
end


------------------------------------------------------------------------------------------------------------------------
function getSpriteOffsetY()
   return (_current_cycle * 2 + (_points_left and 0 or 1)) * SPRITE_SIZE
end


------------------------------------------------------------------------------------------------------------------------
function getMaxCycle()
   return CYCLE_LENGTHS[_current_cycle + 1]
end


------------------------------------------------------------------------------------------------------------------------
function patrol(dt)

   if (_current_cycle ~= CYCLE_WALK) then
      return
   end

   _patrol_time = _patrol_time + dt

   if (_patrol_time > 3000) then
      _patrol_time = 0.0
      _idle = false
   end

   if (_idle) then
      return
   end

   local count = #_patrol_path

   if (count == 0) then
      return
   end

   local key = _patrol_path[_patrol_index]
   local keyVec = v2d.Vector2D(key:getX(), key:getY())

   if (_position:getX() > keyVec:getX() + _patrol_epsilon) then
      goLeft()
   elseif (_position:getX() < keyVec:getX() - _patrol_epsilon) then
      goRight()
   else
      keyReleased(Key["KeyLeft"])
      keyReleased(Key["KeyRight"])

      _patrol_path_arrived = true
      _patrol_index = _patrol_index + 1

      if (_patrol_index > count) then
         _patrol_index = 0
      end

   end
end


------------------------------------------------------------------------------------------------------------------------
function update(dt)
   _elapsed = _elapsed + dt
   decide(dt)
   patrol(dt)
   updateSprite(dt)
   updateKeysPressed(_keys_pressed)
end


------------------------------------------------------------------------------------------------------------------------
function setPath(name, table)

   local i = 0
   local x = 0.0;
   local y = 0.0;
   local v = {}

   for key, value in pairs(table) do

      if ((i % 2) == 0) then
         x = value
      else
         y = value
         v[(i - 1) / 2] = v2d.Vector2D(x, y)
      end

      i = i + 1
   end

   if (name == "path") then
      print("assign patrol path")
      _patrol_path = v
   end
end


------------------------------------------------------------------------------------------------------------------------
function decide(dt)
   elapsed = _current_sprite_elapsed + dt * ANIMATION_SPEEDS[_current_cycle]

   -- it's time for a new cycle
   if (math.floor(elapsed) > getMaxCycle() - 1) then

      next_cycle = _current_cycle
      next_animation_flag_inverse = false

      if (_current_cycle == CYCLE_WALK) then
         next_cycle = CYCLE_WALK
      elseif (_current_cycle == CYCLE_IDLE) then
      elseif (_current_cycle == CYCLE_HIDE) then
      elseif (_current_cycle == CYCLE_HIDE_IDLE) then
      end

      if (next_cycle ~= _current_cycle) then
         _current_cycle = next_cycle
         _animation_flag_inverse = next_animation_flag_inverse
         _current_sprite_elapsed = 0.0
      end
   end
end

