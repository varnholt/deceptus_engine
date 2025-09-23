------------------------------------------------------------------------------------------------------------------------
-- row 1-2 idle
-- row 3-4 walking
-- row 5-6 aim (13 frames)
-- row 7-8 shoot (8 frames)
-- archer shoots only when he stops, he cannot shoot an arrow and walk at the same time
-- so shooting animation splits in two parts, frame 1-13 he gets an arrow from his quiver
-- then we show the 13th frame for a second in order to be fair with the player and has
-- some time to react with the enemy action frame 14-22 is the shoot animation and back to idle position

-- TODO
-- once aim is started, it should not be cancelled when player moves out of scope

require "data/scripts/enemies/constants"
require "data/scripts/enemies/helpers"
local v2d = require "data/scripts/enemies/vectorial2"

------------------------------------------------------------------------------------------------------------------------
-- enemy configuration (constants as locals in lower_case)
local properties = {
   sprite = "data/sprites/enemy_archer.png", -- each frame is 72x72px
   velocity_walk_max = 1.0,
   acceleration_ground = 0.1,
}

local action = {
   idle  = 0,
   walk  = 1,
   aim   = 2,
   shoot = 3,
   die   = 4,
}

-- reverse map (number -> name) for nicer logs
local action_names = {}
for name, id in pairs(action) do
   action_names[id] = name
end

local sprite_counts  = {12, 12, 13, 8, 1}
local sprite_offsets = {0, 144, 288, 432, 576}

local tile_distance_to_follow = 15
local tile_distance_to_aim    = 10
local arrow_speed             = 1.0

-- world-space references
local position        = v2d.Vector2D(0, 0)
local player_position = v2d.Vector2D(0, 0)

-- various
local frame_counter = 0


------------------------------------------------------------------------------------------------------------------------
-- state
local state = {
   action           = action.idle,
   action_prev      = action.idle,

   -- transient flags
   dead             = false,
   waiting          = false,   -- derived from patrol, kept for compatibility via isWaiting()
   aiming_done      = false,   -- aim finished, allowed to shoot
   used_weapon      = false,

   -- inputs/orientation
   key_pressed      = 0,
   points_left      = false,   -- facing left?
   last_points_left = false,   -- for sprite refresh on flip

   -- animation timing
   sprite_time      = math.random(0, 3),
   sprite_index     = 0,

   -- misc
   wait_time        = 0,       -- not used by patrol anymore
   energy           = 20,
}

------------------------------------------------------------------------------------------------------------------------
-- patrol
local patrol = {
   waypoints     = {},   -- { Vector2D, ... }
   index         = 0,    -- 0-based for easier modulo
   epsilon       = 1.0,  -- snap distance on x
   waiting       = false,
   wait_time     = 0.0,
   wait_duration = 3.0,  -- seconds to wait at each point
}

function patrol:reset()
   self.waypoints = {}
   self.index = 0
   self.waiting = false
   self.wait_time = 0.0
   self.epsilon = 1.0
end

function patrol:load_waypoints(list_vec2d)
   self.waypoints = list_vec2d or {}
   self.index = 0
   self.waiting = false
   self.wait_time = 0.0
end

-- helper for setPath(name, table) input
function patrol:load_from_kv_table(tbl)
   local v, i, x = {}, 0, 0.0
   for _, value in pairs(tbl) do
      if (i % 2) == 0 then
         x = value
      else
         v[#v + 1] = v2d.Vector2D(x, value)
      end
      i = i + 1
   end
   self:load_waypoints(v)
end

function patrol:is_waiting()
   return self.waiting
end

function patrol:advance()
   local count = #self.waypoints
   if count == 0 then return end
   self.index = (self.index + 1) % count
end

function patrol:begin_wait()
   self.waiting = true
   self.wait_time = 0.0
end

function patrol:update_wait(dt)
   if not self.waiting then return end
   self.wait_time = self.wait_time + dt
   if self.wait_time >= self.wait_duration then
      self.waiting = false
      self:advance()
   end
end

-- drive movement toward current waypoint; returns true if it handled movement
function patrol:update(dt, self_pos, goLeft, goRight)
   local count = #self.waypoints
   if count == 0 then
      return false
   end

   if self.waiting then
      self:update_wait(dt)
      return true
   end

   local target = self.waypoints[self.index + 1] -- lua arrays are 1-based
   local tx = target:getX()
   local px = self_pos:getX()

   if px > tx + self.epsilon then
      goLeft()
      return true
   elseif px < tx - self.epsilon then
      goRight()
      return true
   else
      -- arrived: begin wait phase
      self:begin_wait()
      -- stop moving while waiting
      state.key_pressed = 0
      return true
   end
end

-- helpers
local function press(key)
   state.key_pressed = (state.key_pressed | key)
end

local function release(key)
   state.key_pressed = (state.key_pressed & (~key))
end

-- change action & reset per-action transients
local function change_action(next_action)

   if next_action == state.action then
      return false
   end

   state.action_prev = state.action
   state.action = next_action
   state.sprite_index = 0
   state.sprite_time  = 0
   state.used_weapon  = false

   return true
end


------------------------------------------------------------------------------------------------------------------------
-- pretty-log all fields of the 'state' table
function log_state(prefix)
   prefix = prefix or "state"

   -- collect & sort keys for stable output
   local keys = {}
   for k in pairs(state) do
      keys[#keys + 1] = k
   end
   table.sort(keys)

   for _, k in ipairs(keys) do
      local v = state[k]
      local t = type(v)

      -- special case for action: print id and name
      if k == "action" and (t == "number" or t == "string") then
         local name = action_names[v] or tostring(v)
         log(prefix .. "." .. k .. " = " .. tostring(v) .. " (" .. name .. ")")
      else
         local out
         if t == "boolean" then
            out = v and "true" or "false"
         elseif t == "number" then
            out = string.format("%.6g", v)
         elseif t == "string" then
            out = string.format("%q", v)
         else
            out = tostring(v)
         end
         log(prefix .. "." .. k .. " = " .. out)
      end
   end
end

------------------------------------------------------------------------------------------------------------------------
function initialize()
   patrol:reset()

   -- collision and sprite setup (same as skeleton)
   addShapeRect(0.2, 0.5, 0.0, 0.23)
   updateSpriteRect(0, 0, 0, 72, 72)
   addHitbox(-18, -18, 36, 48)

   -- weapon: type, interval (ms), damage, gravity, scale, radius
   addWeapon(WeaponType["Bow"], 1, 60, 0.0, 0.1)
end

function retrieveProperties()
   updateProperties(properties)
end

function keyPressed(key)
   press(key)
end

function keyReleased(key)
   release(key)
end

function goLeft()
   state.points_left = true
   release(Key["KeyRight"])
   press(Key["KeyLeft"])
end

function goRight()
   state.points_left = false
   release(Key["KeyLeft"])
   press(Key["KeyRight"])
end

function movedTo(x, y)
   position = v2d.Vector2D(x, y)
end

function playerMovedTo(x, y)
   player_position = v2d.Vector2D(x, y)
end

-- true if our facing matches the player's relative position
function isPointingTowardsPlayer()
   local player_is_left = player_position:getX() < position:getX()
   return player_is_left == state.points_left
end

function followPlayer()
   -- bring player into aim range
   local epsilon = tile_distance_to_aim * 24

   -- |
   -- |
   -- |
   -- |
   -- |            P       < allowed_distance >  E
   -- +-----------------------------------------------------

-- local distance_to_player_y = math.abs(position:getY() // 24 - player_position:getY() // 24)
   -- if distance_to_player_y <= 1 then
   --   end
   if player_position:getX() > position:getX() + epsilon then
      goRight()
   elseif player_position:getX() < position:getX() - epsilon then
      goLeft()
   else
      -- align orientation with player
      state.points_left = player_position:getX() < position:getX()
      state.key_pressed = 0
   end
end

function isWaiting()
   return patrol:is_waiting()
end

function patrol_update(dt)
   local handled = patrol:update(dt, position, goLeft, goRight)
   if not handled then
      state.key_pressed = 0
   end
end

function updateWalk(dt)
   if isPlayerInReach() then
      followPlayer()
   else
      patrol_update(dt)
   end
end

function updateDead(dt)
   if state.sprite_index == sprite_counts[action.die + 1] - 1 then
      die()
   end
end

function updateShoot(dt)
   state.key_pressed = 0

   log("SHOOT!")

   if (isShootPossible()) then

      log(state.sprite_index)

      -- fire once at a specific frame
      if state.sprite_index == sprite_counts[action.shoot + 1] - 1 and not state.used_weapon then
         local vx = state.points_left and -arrow_speed or arrow_speed

         log("shoot")

         useWeapon(
            0,
            position:getX(),
            position:getY(),
            vx,
            0.0
         )

         state.used_weapon = true
      end
   else
      -- new aiming required if shot is no longer possible
      state.aiming_done = false
   end
end

function updateIdle(dt)
   state.key_pressed = 0
end

function isPlayerInReach()
   -- check if player is within range in x direction
   local distance_to_player_x = math.abs(position:getX() // 24 - player_position:getX() // 24)
   return distance_to_player_x < tile_distance_to_follow
end

function hit(damage_value)
   -- todo: apply damage and possibly set state.dead = true
end

function updateAim(dt)
   state.key_pressed = 0

   if (isPointingTowardsPlayer() and isInShootingDistance()) then
      -- allow transition to shoot when last sprite has been reached
      state.aiming_done = (state.sprite_index == sprite_counts[action.aim + 1] - 1)
   else
      state.aiming_done = false
   end
end

function isInShootingDistance()
   if isPointingTowardsPlayer() then
      -- check x distance
      local distance_to_player_x = (position:getX() - player_position:getX()) / 24.0
      if math.abs(distance_to_player_x) <= tile_distance_to_aim then
         -- check y distance
         local distance_to_player_y = math.abs(position:getY() // 24 - player_position:getY() // 24)
         if distance_to_player_y <= 1 then
            return true
         end
      end
   end
   return false
end

function isDead()
   return state.dead
end

function isShootPossible()
   local pointing_towards_player = isPointingTowardsPlayer()
   local in_shooting_distance    = isInShootingDistance()
   local aiming_done             = state.aiming_done

   --   log(string.format(
   --      "shoot check: pointing_towards_player=%s, in_shooting_distance=%s, aiming_done=%s",
   --      tostring(pointing_towards_player),
   --      tostring(in_shooting_distance),
   --      tostring(aiming_done)
   --   ))

   return pointing_towards_player and in_shooting_distance and aim_ready
end

function think()
   local next_action

   if isDead() then
      next_action = action.die
   elseif isShootPossible() then
      next_action = action.shoot
   elseif isInShootingDistance() then
      next_action = action.aim
   elseif not isWaiting() then
      next_action = action.walk
   else
      next_action = action.idle
   end

   local changed = change_action(next_action)
   updateSprite(changed)
end

function act(dt)
   if state.action == action.aim then
      updateAim(dt)
   elseif state.action == action.die then
      updateDead(dt)
   elseif state.action == action.walk then
      updateWalk(dt)
   elseif state.action == action.shoot then
      updateShoot(dt)
   elseif state.action == action.idle then
      updateIdle(dt)
   end
end

function updateSprite(action_changed)
   local count = sprite_counts[state.action + 1]
   local offset = sprite_offsets[state.action + 1]
   local update_required = false

   -- reset sprite index when the action changes
   local next_index
   if action_changed then
      next_index = 0
      update_required = true
   else
      next_index = math.floor(math.fmod(state.sprite_time * 15.0, count))
   end

   -- update sprite index on change
   if next_index ~= state.sprite_index then
      state.sprite_index = next_index
      update_required = true
   end

   -- update when orientation changes (compare with last frame)
   if state.last_points_left ~= state.points_left then
      update_required = true
      state.last_points_left = state.points_left
   end

   -- perform the actual sprite sheet update
   if update_required then
      updateSpriteRect(
         0,
         state.sprite_index * 72,
         offset + (state.points_left and 72 or 0),
         72,
         72
      )
   end
end

function update(dt)
   state.sprite_time = state.sprite_time + dt

   think()
   act(dt)

   updateKeysPressed(state.key_pressed)

   frame_counter = frame_counter + 1
   if frame_counter % 60 == 0 then log_state() end
end

function setPath(name, tbl)
   if name == "path" then
      patrol:load_from_kv_table(tbl)
   end
end
