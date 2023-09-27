require "data/scripts/enemies/constants"
require "data/scripts/enemies/helpers"
v2d = require "data/scripts/enemies/vectorial2"

-- enemy configuration
properties = {
   damage = 0,
   sprite = "data/sprites/rat.png",
   static_body = false,
   velocity_walk_max = 0.75,
   acceleration_ground = 0.1,
   collides_with_player = false
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
CYCLE_UPDOWN = 4
CYCLE_LENGTHS =  {7, 7, 4, 7, 3}
ANIMATION_SPEED = 20.0
DELAY_BETWEEN_IDLE_FRAMES = 1.0


------------------------------------------------------------------------------------------------------------------------
_position = v2d.Vector2D(0, 0)
_player_position = v2d.Vector2D(0, 0)
_points_left = false
_current_cycle = CYCLE_IDLE_1
_current_sprite = 0
_current_sprite_elapsed = 99.0 -- force a sprite update
_patrol_path = {}
_patrol_index = 1
_patrol_epsilon = 1.0
_patrol_path_arrived = false
_elapsed = 0.0
_animation_flag_inverse = false
_key_pressed = 0
_animation_pause_time = 0.0

------------------------------------------------------------------------------------------------------------------------
function initialize()
   addShapeCircle(0.15, 0.0, 0.15)
   updateSprite(0.0)
   addHitbox(-8, 0, 16, 12)
end


------------------------------------------------------------------------------------------------------------------------
function retrieveProperties()
   updateProperties(properties)
end


------------------------------------------------------------------------------------------------------------------------
function playerMovedTo(x, y)
   _player_position = v2d.Vector2D(x, y)
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
function patrol()

   if (_current_cycle ~= CYCLE_RUN) then
      return
   end

   local key = _patrol_path[_patrol_index]
   local key_vec = v2d.Vector2D(key:getX(), key:getY())
   local count = #_patrol_path

   if (_position:getX() > key_vec:getX() + _patrol_epsilon) then
      _points_left = true
      _patrol_path_arrived = false
      keyReleased(Key["KeyRight"])
      keyPressed(Key["KeyLeft"])
   elseif (_position:getX() < key_vec:getX() - _patrol_epsilon) then
      _points_left = false
      _patrol_path_arrived = false
      keyReleased(Key["KeyLeft"])
      keyPressed(Key["KeyRight"])
   else
      keyReleased(Key["KeyLeft"])
      keyReleased(Key["KeyRight"])

      _patrol_path_arrived = true

      _patrol_index = _patrol_index + 1
      if (_patrol_index > count) then
         _patrol_index = 0
      end

      -- update to new alignment after reaching a position
      if (_position:getX() > key_vec:getX() + _patrol_epsilon) then
         _points_left = true
      elseif (_position:getX() < key_vec:getX() - _patrol_epsilon) then
         _points_left = false
      end

   end
end


------------------------------------------------------------------------------------------------------------------------
function decide(dt)
   elapsed = _current_sprite_elapsed + dt * ANIMATION_SPEED

   -- it's time for a new cycle
   if (math.floor(elapsed) > getMaxCycle() - 1) then

      -- possible decisions
      --    run -> stand up
      --    idle 1,2,3 -> random idle or sit down
      --    stand up -> idle 1,2,3
      --    sit down -> run

      next_cycle = _current_cycle
      next_animation_flag_inverse = false

      if (_current_cycle == CYCLE_RUN) then
         if (_patrol_path_arrived) then
            next_cycle = CYCLE_UPDOWN
         else
            next_cycle = CYCLE_RUN
         end

      -- handle what happens after an idle cycle is complete
      elseif (_current_cycle == CYCLE_IDLE_1 or _current_cycle == CYCLE_IDLE_2 or _current_cycle == CYCLE_IDLE_3) then
         _animation_pause_time = DELAY_BETWEEN_IDLE_FRAMES
         rand = math.random(4)
         if (rand == 1) then
            next_cycle = CYCLE_IDLE_1
         elseif (rand == 2) then
            next_cycle = CYCLE_IDLE_2
         elseif (rand == 3) then
            next_cycle = CYCLE_IDLE_3
         elseif (rand == 4) then
            next_cycle = CYCLE_UPDOWN
            next_animation_flag_inverse = true
         end

      -- handle what comes after standing up
      elseif (_current_cycle == CYCLE_UPDOWN and not _animation_flag_inverse) then
         _animation_pause_time = DELAY_BETWEEN_IDLE_FRAMES
         rand = math.random(3)
         if (rand == 1) then
            next_cycle = CYCLE_IDLE_1
         elseif (rand == 2) then
            next_cycle = CYCLE_IDLE_2
         elseif (rand == 3) then
            next_cycle = CYCLE_IDLE_3
         end

      -- handle what comes after sitting down
      elseif (_current_cycle == CYCLE_UPDOWN and _animation_flag_inverse) then
         next_cycle = CYCLE_RUN
      end

      if (next_cycle ~= _current_cycle) then
         _current_cycle = next_cycle
         _animation_flag_inverse = next_animation_flag_inverse
         _current_sprite_elapsed = 0.0
      end
   end
end


------------------------------------------------------------------------------------------------------------------------
function update(dt)
   _elapsed = _elapsed + dt
   decide(dt)
   patrol()
   updateSprite(dt)
   updateKeysPressed(_key_pressed)
end


------------------------------------------------------------------------------------------------------------------------
function getMaxCycle()
   return CYCLE_LENGTHS[_current_cycle + 1]
end


------------------------------------------------------------------------------------------------------------------------
function getSpriteOffsetY()
   return (_current_cycle * 2 + (_points_left and 0 or 1)) * SPRITE_SIZE
end


------------------------------------------------------------------------------------------------------------------------
function updateSprite(dt)

   if (_animation_pause_time > 0.0) then
      _animation_pause_time = _animation_pause_time - dt
      return
   end

   _current_sprite_elapsed = _current_sprite_elapsed + dt * ANIMATION_SPEED
   sprite_index = math.floor(math.fmod(_current_sprite_elapsed, getMaxCycle()))

   if (_current_sprite ~= sprite_index) then
      _current_sprite = sprite_index

      -- sit down is just an inverse stand up cycle
      if (next_animation_flag_inverse) then
         sprite_index = getMaxCycle() - sprite_index - 1
      end

      updateSpriteRect(0, sprite_index * SPRITE_SIZE, getSpriteOffsetY(), SPRITE_SIZE, SPRITE_SIZE)
   end
end


------------------------------------------------------------------------------------------------------------------------
function movedTo(x, y)
   _position = v2d.Vector2D(x, y)
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


