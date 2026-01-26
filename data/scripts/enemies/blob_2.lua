require "data/scripts/enemies/constants"
require "data/scripts/enemies/interpolation"
v2d = require "data/scripts/enemies/vectorial2"


------------------------------------------------------------------------------------------------------------------------
properties = {
   sprite = "data/sprites/enemy_blob_2.png",
   velocity_walk_max = 0.4,
   acceleration_ground = 0.1,
   damage = 4,
   smash = true
}


------------------------------------------------------------------------------------------------------------------------
_patrol_timer = 1
_key_pressed = 0

SPRITE_WIDTH = 72
SPRITE_HEIGHT = 72

-- idle         row 0    15 sprites
-- left         row 1    16 sprites
-- right        row 2    16 sprites
-- jump up      row 3    12 sprites
-- jump down    row 4    12 sprites
-- die          row 5    12 sprites

ROW_IDLE = 0
ROW_LEFT = 1
ROW_RIGHT = 2
ROW_JUMP_UP = 3
ROW_JUMP_DOWN = 4
ROW_DIE = 5
ROW_DROP = 9

SPRITE_COUNT_IDLE = 15
SPRITE_COUNT_LEFT = 16
SPRITE_COUNT_RIGHT = 16
SPRITE_COUNT_JUMP_UP = 12
SPRITE_COUNT_JUMP_DOWN = 12
SPRITE_COUNT_DEATH = 14
SPRITE_COUNT_DROP = 12
SPRITE_COUNT_LANDING = 11 -- last one is skipped, that doesn't make sense

ANIMATION_SPEED = 20.0
IDLE_CYCLE_COUNT = 3

COLLISION_THRESHOLD = 24

_position = v2d.Vector2D(0, 0)
_player_position = v2d.Vector2D(0, 0)
_elapsed = 0.0
_sprite_index = 0
_animation_row = 0
_idle_cycles = 0
_gravity_scale = 1.0
_alignment_offset = 0
_patrol_epsilon = 1.0
_tick = 0
_energy = 30
_dead = false
_smashed = false

-- jump related
_jump = false
_jump_started = false
_jump_height_px = 0
_jump_interval_ms = 0
_jump_start_position = v2d.Vector2D(0, 0)
_jump_time = 0.0
_jump_path = {}

-- drop related
_dropping = false
_drop_prepare = false
_drop_gravity_flipped = false
_drop_time = 0.0
_drop_velocity_exceeded = false
_drop_patrol_velocity_factor = 1.0
_drop_complete = false

-- x: 720..792 (30..33 x 24)
-- y: 984 (41 x 24)


------------------------------------------------------------------------------------------------------------------------
-- interpolation keys
SplineKey = {x = 0, y = 0, time = 0}
function SplineKey:create(o)
  o.parent = self
  return o
end


------------------------------------------------------------------------------------------------------------------------
function initialize()
   _patrol_path = {}
   _patrol_index = 1
   _wait = false

   addShapeCircle(0.12, 0.0, 0.12)                        -- radius, x, y
   addShapeRect(0.2, 0.07, 0.0, 0.1)                      -- width, height, x, y
   updateSpriteRect(0, 0, 0, SPRITE_WIDTH, SPRITE_HEIGHT) -- id, x, y, width, height
   addHitbox(-18, -18, 36, 36)                            -- x offset, y offset, width, height

   -- generate jump path
   k1 = SplineKey:create{x = 0.0, y =  0.0,   time = -1.0}
   k2 = SplineKey:create{x = 0.0, y =  0.01,  time = -0.8}
   k3 = SplineKey:create{x = 0.0, y =  0.07,  time = -0.6}
   k4 = SplineKey:create{x = 0.0, y =  1.0,   time =  0.0}
   k5 = SplineKey:create{x = 0.0, y =  0.0,   time =  0.45}
   k6 = SplineKey:create{x = 0.0, y =  0.0,   time =  1.0}

   _jump_path = {k1, k2, k3, k4, k5, k6}

   _good = true
end


------------------------------------------------------------------------------------------------------------------------
function timeout(id)
   -- print(string.format("timeout: %d", id))
   if (id == _patrol_timer) then
      _wait = false
   end
end


------------------------------------------------------------------------------------------------------------------------
function retrieveProperties()
   updateProperties(properties)
end


------------------------------------------------------------------------------------------------------------------------
function writeProperty(key, value)

   -- print(string.format("write property: %s %s", key, value))

   if (key == "gravity_scale") then
      _gravity_scale = tonumber(value)
      setGravityScale(_gravity_scale)
      _alignment_offset = 6 * 72 - 12
   elseif (key == "jump_height_px") then
      _jump = true
      _jump_height_px = value
   elseif (key == "jump_interval_ms") then
      _jump_interval_ms = value
   elseif (key == "audio_update_behavior") then
      update_behavior = audioUpdateBehaviorFromString(value)
      setAudioUpdateBehavior(update_behavior)
   end
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
function goLeft()
   sprite_index = math.floor(math.fmod(_elapsed * ANIMATION_SPEED, SPRITE_COUNT_LEFT))

   if (_sprite_index ~= sprite_index) then

      _sprite_index = sprite_index
      _animation_row = ROW_LEFT

      updateSpriteRect(
         0,
         _sprite_index * SPRITE_WIDTH,
         _animation_row * SPRITE_HEIGHT + _alignment_offset,
         SPRITE_WIDTH,
         SPRITE_HEIGHT
      )

   end

   keyReleased(Key["KeyRight"])
   keyPressed(Key["KeyLeft"])
end


------------------------------------------------------------------------------------------------------------------------
function goRight()
   sprite_index = math.floor(math.fmod(_elapsed * ANIMATION_SPEED, SPRITE_COUNT_RIGHT))

   if (_sprite_index ~= sprite_index) then

      _sprite_index = sprite_index
      _animation_row = ROW_RIGHT

      updateSpriteRect(
         0,
         _sprite_index * SPRITE_WIDTH,
         _animation_row * SPRITE_HEIGHT + _alignment_offset,
         SPRITE_WIDTH,
         SPRITE_HEIGHT
      )

   end

   keyReleased(Key["KeyLeft"])
   keyPressed(Key["KeyRight"])
end

------------------------------------------------------------------------------------------------------------------------
function updateDeath()

   if (not _dead) then
      return false
   end

   sprite_index = math.floor(math.fmod(_elapsed * ANIMATION_SPEED, SPRITE_COUNT_DEATH + 1))

   if (_sprite_index ~= sprite_index) then

      _sprite_index = sprite_index
      _animation_row = ROW_DIE

      if (sprite_index == SPRITE_COUNT_DEATH) then
         die()
      end

      updateSpriteRect(
         0,
         _sprite_index * SPRITE_WIDTH,
         _animation_row * SPRITE_HEIGHT,
         SPRITE_WIDTH,
         SPRITE_HEIGHT
      )

   end

   return true
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
      _key_pressed = 0
   end
end


------------------------------------------------------------------------------------------------------------------------
function wait()
   -- finish left/right movement animation
   if (_animation_row == ROW_LEFT or _animation_row == ROW_RIGHT) then
      sprite_index = math.floor(math.fmod(_elapsed * ANIMATION_SPEED, SPRITE_COUNT_LEFT))
      if (sprite_index ~= 0) then
         if (_sprite_index ~= sprite_index) then
            _sprite_index = sprite_index
            updateSpriteRect(
               0,
               _sprite_index * SPRITE_WIDTH,
               _animation_row * SPRITE_HEIGHT + _alignment_offset,
               SPRITE_WIDTH,
               SPRITE_HEIGHT
            )
         end
      else
         _animation_row = ROW_IDLE
         _elapsed = 0.0
      end
   else
      -- play idle animation for one cycle
      sprite_index = math.floor(math.fmod(_elapsed * ANIMATION_SPEED, SPRITE_COUNT_IDLE))

      if (_sprite_index ~= sprite_index) then
         _sprite_index = sprite_index
         updateSpriteRect(
            0,
            _sprite_index * SPRITE_WIDTH,
            _animation_row * SPRITE_HEIGHT + _alignment_offset,
            SPRITE_WIDTH,
            SPRITE_HEIGHT
         )

         -- after 1 cycle, go back to business
         if (_sprite_index == 0) then
            _idle_cycles = _idle_cycles + 1

            -- loop idle animation n times
            if (_idle_cycles == IDLE_CYCLE_COUNT) then
               _wait = false
               _idle_cycles = 0
               _elapsed = 0.0
            end
         end
      end
   end
end


------------------------------------------------------------------------------------------------------------------------
function updatePatrol()

   _tick = _tick + 1

   if (_wait == true) then
      -- print(string.format("waiting %d", _tick))
      wait()
      return
   end

   local key = _patrol_path[_patrol_index]

   if (key == nil) then
      print("patrol path is invalid")
      return
   end

   -- update the speed according to the sprite index
   prop = {velocity_walk_max = _drop_patrol_velocity_factor * (math.sin((_sprite_index / SPRITE_COUNT_LEFT) * 6.28318530718) + 1.0) * 0.4}
   updateProperties(prop)

   local keyVec = v2d.Vector2D(key:getX(), key:getY())
   local count = #_patrol_path

   if     (_position:getX() > keyVec:getX() + _patrol_epsilon) then
      -- print(string.format("go left %d", _tick))
      goLeft()
   elseif (_position:getX() < keyVec:getX() - _patrol_epsilon) then
      -- print(string.format("go right %d", _tick))
      goRight()
   else
      -- print(string.format("wait %d", _tick))
      _wait = true
      _key_pressed = 0
      _patrol_index = _patrol_index + 1
      if (_patrol_index > count) then
         _patrol_index = 0
      end
   end
end


------------------------------------------------------------------------------------------------------------------------
function updateJump(dt)

   if (not _jump) then
      return false
   end

   if (not _jump_started) then
      _jump_time = -1.0
      _jump_started = true
   end

   jump_height = 4 * 24
   jump_speed = 1.0

   jump_value = getValueCubic(_jump_path, _jump_time):getY()
   if (jump_value < 0.0) then
      jump_value = 0.0
   end

   y = _jump_start_position:getY() - jump_height * jump_value + 12;

   setTransform(_position:getX(), y, 0.0)

   index = 0
   row = 0

   if (_jump_time < 0.0) then
      index = math.floor((_jump_time + 1.0) * (SPRITE_COUNT_JUMP_UP))
   else
      index = math.floor(_jump_time * SPRITE_COUNT_JUMP_DOWN)
   end

   row = (_jump_time < 0.0) and ROW_JUMP_UP or ROW_JUMP_DOWN

   updateSpriteRect(
      0,
      index * SPRITE_WIDTH,
      row * SPRITE_HEIGHT,
      SPRITE_WIDTH,
      SPRITE_HEIGHT
   )

   _jump_time = _jump_time + dt * jump_speed

   if (_jump_time >= 1.0) then
      _jump_started = false
   end

   return true
end


------------------------------------------------------------------------------------------------------------------------
-- returns true if no subsequent code in the update loop should be executed
function updateDrop(dt)

   if (_drop_complete) then
      return false
   end

   -- check if need to drop
   if (not _drop_prepare and _alignment_offset > 0) then

      dx = _position:getX() - _player_position:getX()

      if (dx > -COLLISION_THRESHOLD and dx < COLLISION_THRESHOLD) then

         -- make sure stone is not too far away (10 tiles) and above player
         y_diff = _position:getY() // 24 - _player_position:getY() // 24

         if (y_diff < 0 and y_diff > -10) then

            -- make sure there's nothing in the way
            if (
               isPhsyicsPathClear(
                  _position:getX(),
                  _position:getY(),
                  _player_position:getX(),
                  _player_position:getY()
               )
            )
            then
               _drop_prepare = true
               return true
            end
         end
      end
   end

   -- check if need to transition sprites to actual drop
   if (_drop_prepare and not _dropping) then

      -- apply a massive slow down on the patrol movement so the blob actually lands on the player
      _drop_patrol_velocity_factor = 0.1

      -- wait until sprite index reaches 0 or 1 so we can transition to drop animation
      if (_sprite_index == 0 or _sprite_index == 1) then
         _dropping = true
         _drop_prepare = false
      else
         -- keep moving until reached the right sprite
         return false
      end
   end

   -- play drop animation
   if (_dropping) then

      _drop_time = _drop_time + dt

      -- first half of the drop animation:
      -- starting to drop and then falling
      if (not mDropLanded) then
         _animation_row = ROW_DROP
         _sprite_index = math.floor(_drop_time * ANIMATION_SPEED)

         -- clamp at end of the column for the whole drop
         if (_sprite_index >= SPRITE_COUNT_DROP) then
            _sprite_index = SPRITE_COUNT_DROP - 1
         end

         if (_sprite_index ~= _sprite_index) then

            _sprite_index = _sprite_index

            -- interpolate from one to the other sprite offset
            -- should go from -12 to +12 from the point the gravity scale
            -- is inverted (that is sprite index 5).
            -- _sprite_index goes from 0..11
            sprite_offset = 0
            if (_sprite_index >= 5) then
               -- 0..6
               sprite_offset = (_sprite_index - 5) * 4
            end
            updateSpriteRect(
               0,
               _sprite_index * SPRITE_WIDTH,
               _animation_row * SPRITE_HEIGHT - (12 - sprite_offset),
               SPRITE_WIDTH,
               SPRITE_HEIGHT
            )

            -- once we reach column 5, the blob is more or less detach from the ceiling,
            -- so now we allow actually letting it fall down
            if (_sprite_index == 5) then
               if (not _drop_gravity_flipped) then
                  setGravityScale(-_gravity_scale)
                  _drop_gravity_flipped = true
               end
            end
         end

         -- as soon as our guy decelerates in y to something close to 0,
         -- we know that it has collided with the ground -> time to draw the landing anim
         velocity = getLinearVelocity()

         if (velocity[2] > 1.0) then
            _drop_velocity_exceeded = true
         end

         if (_drop_velocity_exceeded and velocity[2] <= 0.01) then
            if (not mDropLanded) then
               mDropLanded = true
               _drop_time = 0.0

               -- reset the slow down and move normally
               _drop_patrol_velocity_factor = 1.0
            end
         end

      else

         _animation_row = ROW_JUMP_DOWN
         _alignment_offset = 0

         -- the blob landed on the ground, play the landing animation
         -- also skip the first 5 frames because they somewhat don't make sense
         _sprite_index = 5 + math.floor(_drop_time * ANIMATION_SPEED)

         -- once this animation cycle is complete, the whole drop is done
         if (_sprite_index >= SPRITE_COUNT_LANDING) then
            _sprite_index = SPRITE_COUNT_LANDING - 1
            _drop_complete = true
         end

         if (_sprite_index ~= _sprite_index) then

            _sprite_index = _sprite_index

            updateSpriteRect(
               0,
               _sprite_index * SPRITE_WIDTH,
               _animation_row * SPRITE_HEIGHT,
               SPRITE_WIDTH,
               SPRITE_HEIGHT
            )
         end
      end

      return true
   end

   return false
end


------------------------------------------------------------------------------------------------------------------------
function update(dt)
   _elapsed = _elapsed + dt

   if (updateDeath(dt)) then
      return
   end

   if (updateJump(dt)) then
      return
   end

   if (updateDrop(dt)) then
      return
   end

   updatePatrol()
   updateKeysPressed(_key_pressed)
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
      _patrol_path = v
   end
end



------------------------------------------------------------------------------------------------------------------------
function setStartPosition(x, y)
   _jump_start_position = v2d.Vector2D(x, y)
end



------------------------------------------------------------------------------------------------------------------------
function startDying()
   _dead = true
   _elapsed = 0.0

   -- make sure the thing stops to move
   keyReleased(Key["KeyLeft"])
   keyReleased(Key["KeyRight"])
   prop = {velocity_walk_max = 0.0}
   updateProperties(prop)

   -- when dead, stop causing damage to the player
   setDamage(0)
end


------------------------------------------------------------------------------------------------------------------------
function hit(damage_value)

   if (_dead) then
      return
   end

   -- need to store the current hit time
   -- print(string.format("hit: damage: %d, energy: %d", damage_value, _energy))
   _energy = _energy - damage_value
   if (_energy <= 0) then
      startDying()
   end
end


------------------------------------------------------------------------------------------------------------------------
function smashed()

   if (_smashed) then
      return
   end

   _smashed = true
   startDying()
end
