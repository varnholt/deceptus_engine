-- The sprite sheet enemy_archer.png has 72x72px tiles laid out in pairs of
-- rows for each animation (facing right and facing left). The sprite_counts
-- and sprite_offsets tables below describe how many frames each action
-- contains and where in the sprite sheet those animations begin.  Attack
-- animations use a longer sequence of frames to draw and release the bow.

require "data/scripts/enemies/constants"
require "data/scripts/enemies/helpers"
v2d = require "data/scripts/enemies/vectorial2"

-- enemy configuration
properties = {
   -- use the supplied archer sprite sheet.  Each frame is 72x72px
   sprite = "data/sprites/enemy_archer.png",
   velocity_walk_max = 1.0,
   acceleration_ground = 0.1
}

-- enumeration for the different action states.  These map directly to
-- indices in the sprite_counts/sprite_offsets tables below.
action = {
   walk   = 0,
   idle   = 1,
   hit    = 2,
   die    = 3,
   attack = 4
}

------------------------------------------------------------------------------------------------------------------------
-- internal state variables
patrol_timer    = 1
key_pressed     = 0
position        = v2d.Vector2D(0, 0)
player_position = v2d.Vector2D(0, 0)
points_left     = false
sprite_time     = math.random(0, 3)
time_accum      = 0
sprite_index    = 0

-- The sprite sheet is organized in pairs of rows: for each action the
-- first row contains frames when the enemy is facing right and the second
-- row contains frames when facing left.  The following tables tell the
-- engine how many frames belong to each action and at what vertical offset
-- (in pixels) the animation starts.  Offsets are multiples of 72 since
-- each frame is 72px high.  The mapping of actions to row pairs is as
-- follows:
--   walk   -> rows 0 & 1  (12 frames)
--   idle   -> rows 2 & 3  (12 frames)
--   hit    -> rows 6 & 7  (7 frames; only the first row has content)
--   die    -> rows 8 & 9  (5 frames; only the first row has content)
--   attack -> rows 4 & 5  (20 frames; used for drawing and releasing the bow)
sprite_counts  = {12, 12, 7, 5, 20}
sprite_offsets = {
   0,    -- walk   starts at row 0  (0 * 72)
   144,  -- idle   starts at row 2  (2 * 72)
   432,  -- hit    starts at row 6  (6 * 72)
   576,  -- die    starts at row 8  (8 * 72)
   288   -- attack starts at row 4  (4 * 72)
}

current_action  = action["idle"]
energy          = 20
dead            = false
waiting         = false
attack_started  = false
attack_launched = false
is_hit          = false

-- Arrow speed used when firing from the bow.  Positive values shoot to
-- the right, negative values shoot to the left.
arrow_speed = 1000.0

------------------------------------------------------------------------------------------------------------------------
function initialize()
   -- reset patrol state
   patrol_path     = {}
   patrol_index    = 0
   patrol_epsilon  = 1.0

   -- collision and sprite setup (same as skeleton)
   addShapeRect(0.2, 0.5, 0.0, 0.23)
   updateSpriteRect(0, 0, 0, 72, 72)
   addHitbox(-18, -18, 36, 48)

   -- equip the bow weapon.  The parameters are: type, interval (ms),
   -- damage, gravity scale and radius.  These values mirror those used
   -- by arrow traps in the game.
   addWeapon(WeaponType["Bow"], 50, 60, 0.0, 0.1)
end

------------------------------------------------------------------------------------------------------------------------
function timeout(id)
   -- simply reset waiting when the patrol timer fires
   if (id == patrol_timer) then
      waiting = false
   end
end

------------------------------------------------------------------------------------------------------------------------
function retrieveProperties()
   updateProperties(properties)
end

------------------------------------------------------------------------------------------------------------------------
function keyPressed(key)
   key_pressed = (key_pressed | key)
end

------------------------------------------------------------------------------------------------------------------------
function keyReleased(key)
   key_pressed = key_pressed & (~key)
end

------------------------------------------------------------------------------------------------------------------------
function goLeft()
   points_left = true
   keyReleased(Key["KeyRight"])
   keyPressed(Key["KeyLeft"])
end

------------------------------------------------------------------------------------------------------------------------
function goRight()
   points_left = false
   keyReleased(Key["KeyLeft"])
   keyPressed(Key["KeyRight"])
end

------------------------------------------------------------------------------------------------------------------------
function movedTo(x, y)
   position = v2d.Vector2D(x, y)
end

------------------------------------------------------------------------------------------------------------------------
function playerMovedTo(x, y)
   player_position = v2d.Vector2D(x, y)
end

------------------------------------------------------------------------------------------------------------------------
-- follow the player horizontally if within reach
function followPlayer()
   local epsilon = 5
   if (player_position:getX() > position:getX() + epsilon) then
      goRight()
   elseif (player_position:getX() < position:getX() - epsilon) then
      goLeft()
   else
      key_pressed = 0
   end
end

------------------------------------------------------------------------------------------------------------------------
function wait()
   if (waiting) then
      return
   end

   -- count entries explicitly (0-based array -> # is unreliable)
   local count = 0
   for _ in pairs(patrol_path) do
      count = count + 1
   end

   -- no path -> nothing to wait/patrol for
   if (count == 0) then
      key_pressed = 0
      return
   end

   waiting = true
   key_pressed = 0
   timer(3000, patrol_timer)

   patrol_index = patrol_index + 1
   if (patrol_index >= count) then   -- wrap for 0..count-1
      patrol_index = 0
   end
end


------------------------------------------------------------------------------------------------------------------------
function patrol()
   local key = patrol_path[patrol_index]
   if not key then
      -- path not set yet or index out of bounds -> idle this tick
      key_pressed = 0
      return
   end

   local key_vec = v2d.Vector2D(key:getX(), key:getY())
   if     (position:getX() > key_vec:getX() + patrol_epsilon) then
      goLeft()
   elseif (position:getX() < key_vec:getX() - patrol_epsilon) then
      goRight()
   else
      wait()
   end
end


------------------------------------------------------------------------------------------------------------------------
function walk()
   if (isPlayerInReach()) then
      followPlayer()
   else
      patrol()
   end
end

------------------------------------------------------------------------------------------------------------------------
function act()
   if (current_action == action["hit"]) then
      updateHit()
   elseif (current_action == action["die"]) then
      updateDead()
   elseif (current_action == action["walk"]) then
      walk()
   elseif (current_action == action["attack"]) then
      attack()
   end
end

------------------------------------------------------------------------------------------------------------------------
-- when dying just wait for the last death frame then remove
function updateDead()
   if (sprite_index == sprite_counts[action["die"] + 1] - 1) then
      die()
   end
end

------------------------------------------------------------------------------------------------------------------------
-- attack behaviour for the archer.  During the attack animation the
-- archer draws the bow and releases an arrow.  When sprite_index
-- reaches a particular frame we spawn an arrow using useGun().  After the
-- animation finishes the attack flag is reset.
function attack()
   -- stop moving during an attack
   key_pressed = 0
   attack_started = true
   -- choose a frame roughly half way through the animation to release
   -- the arrow.  For a 20 frame attack sequence frame 10 is a good
   -- compromise between draw and release.
   if (sprite_index == 10) then
      if (not attack_launched) then
         attack_launched = true
         -- compute initial projectile velocity based on facing direction
         local vx = points_left and -arrow_speed or arrow_speed
         local vy = 0.0
         useGun(
            0,
            position:getX(),
            position:getY(),
            vx,
            vy
         )
      end
   else
      -- reset state when the animation has finished
      if (sprite_index == sprite_counts[action["attack"] + 1] - 1) then
         attack_started = false
      end
      attack_launched = false
   end
end

------------------------------------------------------------------------------------------------------------------------
-- determine whether the player is close enough to follow
function isPlayerInReach()
   local in_reach = false
   -- check if player is within range in x direction
   local distance_to_player_x = position:getX() // 24 - player_position:getX() // 24
   if (math.abs(distance_to_player_x) < 10) then
      -- y direction check can be expanded if needed
      in_reach = true
   end
   return in_reach
end

------------------------------------------------------------------------------------------------------------------------
-- called when this enemy takes damage
function hit(damage_value)
   if (not is_hit) then
      is_hit = true
      key_pressed = 0
      energy = energy - damage_value
      if (energy <= 0) then
         dead = true
      end
   end
end

------------------------------------------------------------------------------------------------------------------------
-- update the hit animation and reset the hit flag when finished
function updateHit()
   if (is_hit) then
      if (sprite_index >= sprite_counts[action["hit"] + 1] - 1) then
         is_hit = false
      end
   end
end

------------------------------------------------------------------------------------------------------------------------
function isHit()
   return is_hit
end

------------------------------------------------------------------------------------------------------------------------
function isWaiting()
   return waiting
end

------------------------------------------------------------------------------------------------------------------------
-- Determine whether the enemy can attack.  For ranged enemies we keep the
-- same conditions as the skeleton: we only attack if the player is close
-- enough horizontally or if an attack is currently in progress.  This
-- prevents the archer from firing repeatedly after starting an attack.
function checkAttackDistance()
   local distance_to_player_x = (position:getX() - player_position:getX()) / 24.0
   if (math.abs(distance_to_player_x) <= 1.5) then
      local distance_to_player_y = position:getY() // 24 - player_position:getY() // 24
      if (math.abs(distance_to_player_y) <= 1) then
         return true
      end
   end
   return false
end

------------------------------------------------------------------------------------------------------------------------
function canAttack()
   if (attack_started) then
      return true
   end
   return checkAttackDistance()
end

------------------------------------------------------------------------------------------------------------------------
-- Decide which animation state we should be in for this frame
function think()
   local next_action = current_action
   if (dead) then
      next_action = action["die"]
   elseif (isHit()) then
      next_action = action["hit"]
   elseif (canAttack()) then
      next_action = action["attack"]
   elseif (not isWaiting()) then
      next_action = action["walk"]
   else
      next_action = action["idle"]
   end
   local changed = (next_action ~= current_action)
   updateSprite(changed)
   current_action = next_action
end

------------------------------------------------------------------------------------------------------------------------
-- Update sprite indices based on current action, facing direction and time
function updateSprite(changed)
   local sprite_count = sprite_counts[current_action + 1]
   local sprite_offset = sprite_offsets[current_action + 1]
   local update_required = false
   local points_left_prev = points_left
   -- reset sprite index when the action changes
   local sprite_index_local
   if (changed) then
      sprite_index_local = 0
      update_required = true
      sprite_time = 0
   else
      sprite_index_local = math.floor(math.fmod(sprite_time * 15.0, sprite_count))
   end
   -- update sprite index on change
   if (sprite_index_local ~= sprite_index) then
      sprite_index = sprite_index_local
      update_required = true
   end
   -- update when direction changes
   if (points_left_prev ~= points_left) then
      update_required = true
   end
   -- perform the actual sprite sheet update
   if (update_required) then
      updateSpriteRect(
         0,
         sprite_index * 72,
         sprite_offset + (points_left and 72 or 0),
         72,
         72
      )
   end
end

------------------------------------------------------------------------------------------------------------------------
function update(dt)
   time_accum  = time_accum + dt
   sprite_time = sprite_time + dt
   think()
   act()
   updateKeysPressed(key_pressed)
end

------------------------------------------------------------------------------------------------------------------------
-- Parse patrol path definitions passed from the level editor
function setPath(name, table)
   local i = 0
   local x = 0.0
   local y = 0.0
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
      patrol_path = v
   end
end
