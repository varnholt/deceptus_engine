require "data/scripts/enemies/constants"
require "data/scripts/enemies/helpers"
v2d = require "data/scripts/enemies/vectorial2"

-- enemy configuration
properties = {
   damage = 0,
   sprite = "data/sprites/rat.png",
   staticBody = false
}


-- row 0-1: run
-- row 2-3: tail move (idle)
-- row 4-5: nose move (idle)
-- row 6-7: blink (idle)
-- row 8-9: stand up / sit down (from run to idle and idle to run)
--
-- run -> stand up -> idle 1/2/3
-- idle 1/2/3 -> sit -> run
--
-- even: rat points left
--
-- 0: 7 RUN
-- 2: 7 IDLE 1
-- 4: 4 IDLE 2
-- 6: 7 IDLE 3
-- 8: 2 UP/DOWN

SPRITE_SIZE = 24
CYCLE_RUN = 0
CYCLE_IDLE_1 = 1
CYCLE_IDLE_2 = 2
CYCLE_IDLE_3 = 3
CYCLE_STAND_UP = 4
CYCLE_LENGTHS =  {7, 7, 4, 7, 2}


------------------------------------------------------------------------------------------------------------------------
_start_position = v2d.Vector2D(0, 0)
_position = v2d.Vector2D(0, 0)
_player_position = v2d.Vector2D(0, 0)
_points_left = false
_current_cycle = CYCLE_IDLE_1
_current_sprite = 0
_current_sprite_elapsed = 0.0
_patrol_path = {}
_patrol_index = 1
_patrol_epsilon = 1.0
_elapsed = 0.0


------------------------------------------------------------------------------------------------------------------------
function initialize()
   addShapeCircle(0.05, 0.0, 0.0)
   updateSprite(0.0)
end


------------------------------------------------------------------------------------------------------------------------
function timeout(id)
   if (id == _patrol_timer) then
   end
end


------------------------------------------------------------------------------------------------------------------------
function retrieveProperties()
   updateProperties(properties)
end


------------------------------------------------------------------------------------------------------------------------
function goLeft()
   _points_left = true
end


------------------------------------------------------------------------------------------------------------------------
function goRight()
   _points_left = false
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
   end
end


------------------------------------------------------------------------------------------------------------------------
function patrol()

   if (wait == true) then
      return
   end

   local key = _patrol_path[_patrol_index]
   local key_vec = v2d.Vector2D(key:getX(), key:getY())
   local count = #_patrol_path

   if (_position:getX() > key_vec:getX() + _patrol_epsilon) then
      goLeft()
   elseif (_position:getX() < key_vec:getX() - _patrol_epsilon) then
      goRight()
   else
--      timer(5000, _patrol_timer)
--      _patrol_index = _patrol_index + 1
--      if (_patrol_index > count) then
--         _patrol_index = 0
--      end
   end
end


------------------------------------------------------------------------------------------------------------------------
function decide()
   -- if (_current_sprite  == CYCLE_LENGTHS[_current_cycle + 1])
   -- end
end


------------------------------------------------------------------------------------------------------------------------
function update(dt)
   -- patrol()
   -- setTransform(_start_position:getX(), _start_position:getY(), 0.0)
   _elapsed = _elapsed + dt
   decide()
   updateSprite(dt)
end


------------------------------------------------------------------------------------------------------------------------
function updateSprite(dt)
   _current_sprite_elapsed = _current_sprite_elapsed + dt

   max_cycle = CYCLE_LENGTHS[_current_cycle + 1]
   sprite_index = math.floor(math.fmod(_current_sprite_elapsed, max_cycle))

   if (_current_sprite ~= sprite_index) then
      print(sprite_index)
      _current_sprite = sprite_index
      updateSpriteRect(0, _current_sprite * SPRITE_SIZE, 0, SPRITE_SIZE, SPRITE_SIZE)
   end
end


------------------------------------------------------------------------------------------------------------------------
function movedTo(x, y)
   _position = v2d.Vector2D(x, y)
   -- print(string.format("rat position: %f, %f", x, y))
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
      _patrol_path = v
   end
end


------------------------------------------------------------------------------------------------------------------------
function setStartPosition(x, y)
   print(string.format("rat start position: %f, %f", x, y))
   _start_position = v2d.Vector2D(x, y)
end
