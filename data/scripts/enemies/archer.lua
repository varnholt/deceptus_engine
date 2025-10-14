------------------------------------------------------------------------------------------------------------------------
-- row 1-2 idle
-- row 3-4 walking
-- row 5-6 aim (13 frames)
-- row 7-8 shoot (8 frames)
-- archer shoots only when he stops, he cannot shoot an arrow and walk at the same time
-- so shooting animation splits in two parts, frame 1-13 he gets an arrow from his quiver
-- then we show the 13th frame for a second in order to be fair with the player and has
-- some time to react with the enemy action frame 14-22 is the shoot animation and back to idle position
--
-- aiming and shooting
-- once aim is started, it should not be cancelled when player moves out of scope
--
-- after shooting
-- player should go into idle for a certain period
------------------------------------------------------------------------------------------------------------------------

-- ---------------------------------------------------------------------------------------------------------------------
-- state machine (archer)
--
--           +-------------------------+
--           |                         v
--         patrol  +----->  patrol_wait  ->  aim  ->  aim_wait  ->  shoot  ->  recover
--           ^  \                      ^                         ^                |
--           |   \                     |                         |                |
--           |    \--------------------+-------------------------+----------------+
--           |                                (no interrupt during aim)
--           |  (walk a<->b; if no path -> idle pose)
--           +--------------------------------------------------------------------+
--                                          after recover: either aim (if still valid)
--                                          or continue patrolling
-- ---------------------------------------------------------------------------------------------------------------------
-- we explicitly show the flow here so design intent is obvious without reading code.
-- “aim is never interrupted” is enforced by transitions, not flags.

require "data/scripts/enemies/constants"
require "data/scripts/enemies/helpers"
local vector2       = require "data/scripts/enemies/vectorial2"
local StateMachine  = require "data/scripts/enemies/state_machine" -- dedicated reusable state machine

------------------------------------------------------------------------------------------------------------------------
-- enemy configuration (constants as locals)
-- bundling constants in one place makes balancing and tuning easier.
local archer_properties = {
   sprite_path         = "data/sprites/enemy_archer.png", -- each frame is 72x72px
   maximum_walk_speed  = 1.0,
   ground_acceleration = 0.1,
}

-- state ids: sprite-backed + logical states (the last three reuse sprite rows via aliasing)
local state_id = {
   idle        = 0,
   walk        = 1,
   aim         = 2,
   shoot       = 3,
   die         = 4,
   patrol_wait = 5,
   aim_wait    = 6,
   recover     = 7,
}

-- id->name map for readable logging
local state_names = {}
for name, id in pairs(state_id) do state_names[id] = name end

-- sprite meta (first 5 are real rows; logical states are drawn via aliases below)
-- we prefer not to duplicate sprite rows for timing-only states like aim_wait/recover.
local sprite_frame_counts  = {12, 12, 13, 8, 1, 1, 1, 12}
local sprite_row_offsets   = {0, 144, 288, 432, 576, 0, 0, 0}

-- how logical states map to real rows
-- “aim_wait” keeps the final aim frame visible; “recover” idles but with a timed lockout.
local sprite_alias = {
   [state_id.patrol_wait] = { base = state_id.idle,  index = 0,   loop = false }, -- hold idle frame
   [state_id.aim_wait]    = { base = state_id.aim,   index = 12,  loop = false }, -- hold last aim frame (0-based 12)
   [state_id.recover]     = { base = state_id.idle,  index = nil, loop = true  }, -- idle loop during recovery
}

-- tuning knobs (units are tiles/seconds where possible)
-- these are easy to surface for designers later without touching logic.
local follow_distance_tiles   = 15
local aim_distance_tiles      = 10
local arrow_speed_units       = 1.0
local aim_hold_duration_s     = 1.0
local recover_duration_s      = 2.0

-- world-space references (kept here to avoid global lookups and simplify testing)
local archer_position         = vector2.Vector2D(0, 0)
local player_position         = vector2.Vector2D(0, 0)

-- visual state (sprite, input, orientation)
-- we keep rendering and input intent here, separate from ai logic in the state machine.
local archer_visual_state = {
   action_id        = state_id.idle,
   previous_action  = state_id.idle,

   key_pressed_mask = 0,
   facing_left      = false,
   last_facing_left = false,

   animation_time_s = 0.0,
   sprite_time_s    = math.random(0, 3),
   sprite_index     = 0,

   energy           = 20,   -- placeholder for future combat logic
}

------------------------------------------------------------------------------------------------------------------------
-- patrol behavior
-- we isolate waypoint logic so the state machine only decides “walk vs wait”, not how to walk.
local patrol_behavior = {
   waypoints     = {},
   index_zero    = 0,    -- 0-based index to make modulo wrap easier
   epsilon_x     = 1.0,  -- positional tolerance before we declare arrival
   is_waiting    = false,
   waited_s      = 0.0,
   wait_duration = 3.0,  -- time to pause at a waypoint
}

function patrol_behavior:reset()
   self.waypoints  = {}
   self.index_zero = 0
   self.is_waiting = false
   self.waited_s   = 0.0
   self.epsilon_x  = 1.0
end

function patrol_behavior:load_waypoints(list_vec2d)
   self.waypoints  = list_vec2d or {}
   self.index_zero = 0
   self.is_waiting = false
   self.waited_s   = 0.0
end

-- helper for setPath(name, table)
function patrol_behavior:load_from_kv_table(table_kv)
   -- the engine gives us a flat kv array: x1,y1,x2,y2,... so we rebuild points here.
   local points, index, pending_x = {}, 0, 0.0
   for _, value in pairs(table_kv) do
      if (index % 2) == 0 then
         pending_x = value
      else
         points[#points + 1] = vector2.Vector2D(pending_x, value)
      end
      index = index + 1
   end
   self:load_waypoints(points)
end

function patrol_behavior:start_wait()
   -- we centralize wait bookkeeping to keep state code lean.
   self.is_waiting = true
   self.waited_s   = 0.0
end

function patrol_behavior:update_wait(delta_time)
   if not self.is_waiting then return end
   self.waited_s = self.waited_s + delta_time
   if self.waited_s >= self.wait_duration then
      self.is_waiting = false
      local count = #self.waypoints
      if count > 0 then
         self.index_zero = (self.index_zero + 1) % count
      end
   end
end

-- returns true if it handled movement; false means no path configured
function patrol_behavior:update_move(delta_time, self_position, move_left, move_right)
   local count = #self.waypoints
   if count == 0 then
      return false  -- no path -> let the state machine decide between idle/aim
   end

   if self.is_waiting then
      self:update_wait(delta_time)
      return true
   end

   local target      = self.waypoints[self.index_zero + 1] -- lua arrays are 1-based
   local target_x    = target:getX()
   local position_x  = self_position:getX()

   if position_x > target_x + self.epsilon_x then
      move_left();  return true
   elseif position_x < target_x - self.epsilon_x then
      move_right(); return true
   else
      self:start_wait()
      archer_visual_state.key_pressed_mask = 0
      return true
   end
end

------------------------------------------------------------------------------------------------------------------------
-- input helpers (kept tiny on purpose)
local function press_key(key)
   archer_visual_state.key_pressed_mask = (archer_visual_state.key_pressed_mask | key)
end

local function release_key(key)
   archer_visual_state.key_pressed_mask = (archer_visual_state.key_pressed_mask & (~key))
end

function keyPressed(key)  press_key(key)   end
function keyReleased(key) release_key(key) end

function move_left()
   archer_visual_state.facing_left = true
   release_key(Key["KeyRight"])
   press_key(Key["KeyLeft"])
end

function move_right()
   archer_visual_state.facing_left = false
   release_key(Key["KeyLeft"])
   press_key(Key["KeyRight"])
end

function movedTo(x, y)
   archer_position = vector2.Vector2D(x, y)
end

function playerMovedTo(x, y)
   player_position = vector2.Vector2D(x, y)
end

-- align sprite orientation to the player's side (no movement)
local function align_orientation_towards_player()
   archer_visual_state.facing_left = (player_position:getX() < archer_position:getX())
end

------------------------------------------------------------------------------------------------------------------------
-- sensing and small decision helpers
-- we keep these pure and reusable so transition rules stay readable.

local function is_facing_player()
   local player_is_left = player_position:getX() < archer_position:getX()
   return player_is_left == archer_visual_state.facing_left
end

local function follow_player()
   -- move just enough to bring player into aim range, then stop and face them.
   local epsilon_pixels = aim_distance_tiles * 24
   if player_position:getX() > archer_position:getX() + epsilon_pixels then
      move_right()
   elseif player_position:getX() < archer_position:getX() - epsilon_pixels then
      move_left()
   else
      archer_visual_state.facing_left = player_position:getX() < archer_position:getX()
      archer_visual_state.key_pressed_mask = 0
   end
end

local function is_player_in_follow_range()
   local dx_tiles = math.abs(archer_position:getX() // 24 - player_position:getX() // 24)
   return dx_tiles < follow_distance_tiles
end

local function is_player_in_shoot_range()
   if not is_facing_player() then return false end
   local dx_tiles = (archer_position:getX() - player_position:getX()) / 24.0
   if math.abs(dx_tiles) > aim_distance_tiles then return false end
   local dy_tiles = math.abs(archer_position:getY() // 24 - player_position:getY() // 24)
   return dy_tiles <= 1
end

local function is_archer_dead()
   -- todo: connect to health/damage; this stays here so we can also reuse the state machine for other enemies
   return false
end

------------------------------------------------------------------------------------------------------------------------
-- optional logging
-- stable sorted output makes diffs readable and debugging predictable.
function log_state(prefix)
   prefix = prefix or "archer"
   local keys = {}
   for key in pairs(archer_visual_state) do keys[#keys + 1] = key end
   table.sort(keys)
   for _, key in ipairs(keys) do
      local value = archer_visual_state[key]
      local kind  = type(value)
      if key == "action_id" and (kind == "number" or kind == "string") then
         local name = state_names[value] or tostring(value)
         log(prefix .. "." .. key .. " = " .. tostring(value) .. " (" .. name .. ")")
      else
         local out
         if kind == "boolean" then
            out = value and "true" or "false"
         elseif kind == "number" then
            out = string.format("%.6g", value)
         elseif kind == "string" then
            out = string.format("%q", value)
         else
            out = tostring(value)
         end
         log(prefix .. "." .. key .. " = " .. out)
      end
   end
end

------------------------------------------------------------------------------------------------------------------------
-- build the state machine
-- states do side effects; transitions encode the “story” and constraints declaratively.

local archer_state_machine = StateMachine.new(state_id.idle)

-- small shoot-state flag
-- “shoot” needs a one-shot side effect (firing an arrow) on the first frame only.
local shot_fired_in_this_cycle = false

-- state registrations (lightweight: effect-only; most branching is in transition rules)
archer_state_machine
:add_state(state_id.walk, {
   on_update = function(delta_time, sm)
      if is_player_in_follow_range() then
         follow_player()
      else
         local handled = patrol_behavior:update_move(delta_time, archer_position, move_left, move_right)
         if not handled then
            archer_visual_state.key_pressed_mask = 0
         end
      end
   end,
})
:add_state(state_id.patrol_wait, {
   on_enter  = function()
      patrol_behavior:start_wait()
      archer_visual_state.key_pressed_mask = 0
   end,

on_update = function(delta_time, sm)

      if is_player_in_follow_range() then
         align_orientation_towards_player()
      end

      archer_visual_state.key_pressed_mask = 0
      patrol_behavior:update_wait(delta_time)
   end,
})
:add_state(state_id.aim, {
   on_enter  = function()
      -- “aim” cannot be interrupted; we rely on animation completion to move on.
      archer_visual_state.key_pressed_mask = 0
      shot_fired_in_this_cycle = false
   end,
   on_update = function(delta_time, sm)
      archer_visual_state.key_pressed_mask = 0
   end,
})
:add_state(state_id.aim_wait, {
   on_enter  = function() archer_visual_state.key_pressed_mask = 0 end,
   on_update = function(delta_time, sm) archer_visual_state.key_pressed_mask = 0 end,
})
:add_state(state_id.shoot, {
   on_enter  = function()
      archer_visual_state.key_pressed_mask = 0
      shot_fired_in_this_cycle = false
   end,
   on_update = function(delta_time, sm)
      -- we only spawn the arrow once at the start of the shoot row for deterministic gameplay.
      archer_visual_state.key_pressed_mask = 0
      if not shot_fired_in_this_cycle and archer_visual_state.sprite_index == 0 then
         local velocity_x = archer_visual_state.facing_left and -arrow_speed_units or arrow_speed_units
         useWeapon(0, archer_position:getX(), archer_position:getY(), velocity_x, 0.0)
         shot_fired_in_this_cycle = true
      end
   end,
})
:add_state(state_id.recover, {
   on_enter  = function()
      -- recovery intentionally locks actions for a short time to telegraph vulnerability.
      archer_visual_state.key_pressed_mask = 0
   end,
   on_update = function(delta_time, sm)
      archer_visual_state.key_pressed_mask = 0
   end,
})
:add_state(state_id.idle, {
   on_update = function(delta_time, sm)

      -- keep idle responsive: if player is nearby, at least face them
      if is_player_in_follow_range() then
         align_orientation_towards_player()
      end

      -- idle still updates patrol wait so newly assigned paths kick in without a state bounce.
      archer_visual_state.key_pressed_mask = 0
      patrol_behavior:update_wait(delta_time)
   end,
})
:add_state(state_id.die, {
   on_update = function(delta_time, sm)
      -- wait until the death row is finished to remove the entity for a clean visual exit.
      if archer_visual_state.sprite_index == sprite_frame_counts[state_id.die + 1] - 1 then
         die()
      end
   end,
})

-- tiny helpers used by guards
local function has_finished_aim_row()
   return archer_visual_state.sprite_index >= (sprite_frame_counts[state_id.aim + 1] - 1)
end

local function has_finished_shoot_row()
   return archer_visual_state.sprite_index >= (sprite_frame_counts[state_id.shoot + 1] - 1)
end

-- transition rules (priority order matters within each list)
-- these read like english rules and can be tweaked without touching state code.

archer_state_machine
:add_transition_rules(state_id.walk, {
   { to = state_id.aim,        when = function(_) return is_player_in_follow_range() and is_player_in_shoot_range() end },
   { to = state_id.patrol_wait,when = function(_) return patrol_behavior.is_waiting end },
   { to = state_id.idle,       when = function(_) return #patrol_behavior.waypoints == 0 end },
})
:add_transition_rules(state_id.patrol_wait, {
   { to = state_id.aim,  when = function(_) return is_player_in_shoot_range() end },
   { to = state_id.walk, when = function(_) return not patrol_behavior.is_waiting end },
})
:add_transition_rule(state_id.aim,      state_id.aim_wait,  function(_) return has_finished_aim_row() end)
:add_transition_rule(state_id.aim_wait, state_id.shoot,     function(sm) return sm.elapsed_time_in_state >= aim_hold_duration_s end)
:add_transition_rule(state_id.shoot,    state_id.recover,   function(_)  return has_finished_shoot_row() end)
:add_transition_rules(state_id.recover, {
   { to = state_id.aim,  when = function(sm) return sm.elapsed_time_in_state >= recover_duration_s and is_player_in_shoot_range() end },
   { to = state_id.walk, when = function(sm) return sm.elapsed_time_in_state >= recover_duration_s and not is_player_in_shoot_range() and #patrol_behavior.waypoints > 0 end },
   { to = state_id.idle, when = function(sm) return sm.elapsed_time_in_state >= recover_duration_s and not is_player_in_shoot_range() and #patrol_behavior.waypoints == 0 end },
})
:add_transition_rules(state_id.idle, {
   { to = state_id.aim,  when = function(_) return is_player_in_shoot_range() end },
   { to = state_id.walk, when = function(_) return #patrol_behavior.waypoints > 0 and not patrol_behavior.is_waiting end },
})

-- global rule (checked for all states)
archer_state_machine:add_global_rule(state_id.die, function(_) return is_archer_dead() end)

------------------------------------------------------------------------------------------------------------------------
-- sprite update with alias support
-- rendering concerns stay separate from state logic; aliasing avoids duplicate sprite rows.

local function update_sprite_frame(action_changed)
   archer_visual_state.previous_action = archer_visual_state.action_id
   archer_visual_state.action_id       = archer_state_machine.current_state_id

   if action_changed then
      archer_visual_state.animation_time_s = 0.0
   end

   local current_id   = archer_visual_state.action_id
   local alias        = sprite_alias[current_id]
   local draw_row_id  = current_id
   local forced_index = nil
   local is_non_loop  = (current_id == state_id.aim) or (current_id == state_id.shoot)

   if alias then
      draw_row_id  = alias.base
      is_non_loop  = not alias.loop
      forced_index = alias.index
   end

   local frame_count  = sprite_frame_counts[draw_row_id + 1]
   local row_offset   = sprite_row_offsets[draw_row_id + 1]
   local needs_update = false

   local next_index
   if action_changed then
      next_index   = forced_index or 0
      needs_update = true
   else
      if forced_index ~= nil then
         next_index = forced_index
      else
         -- sprites run at 15 fps; we derive frame from time to avoid manual counters.
         local frame_candidate = math.floor(archer_visual_state.animation_time_s * 15.0)
         if is_non_loop then
            next_index = math.min(frame_candidate, frame_count - 1)
         else
            next_index = math.floor(math.fmod(frame_candidate, frame_count))
         end
      end
   end

   if next_index ~= archer_visual_state.sprite_index then
      archer_visual_state.sprite_index = next_index
      needs_update = true
   end

   if archer_visual_state.last_facing_left ~= archer_visual_state.facing_left then
      needs_update = true
      archer_visual_state.last_facing_left = archer_visual_state.facing_left
   end

   if needs_update then
      updateSpriteRect(
         0,
         archer_visual_state.sprite_index * 72,
         row_offset + (archer_visual_state.facing_left and 72 or 0),
         72,
         72
      )
   end
end

------------------------------------------------------------------------------------------------------------------------
-- engine entry points
-- these match the engine expectations and keep setup/update pathways obvious.

function initialize()
   patrol_behavior:reset()

   addShapeRect(0.2, 0.5, 0.0, 0.23)
   updateSpriteRect(0, 0, 0, 72, 72)
   addHitbox(-18, -18, 36, 48)

   -- weapon: type, interval (ms), damage, gravity, scale, radius
   -- the bow is configured once here; shot velocity is decided at fire time.
   addWeapon(WeaponType["Bow"], 1, 60, 0.0, 0.1)

   update_sprite_frame(true)
end

function retrieveProperties()
   -- the engine reads properties from here; we translate our named config to the expected keys.
   updateProperties({
      sprite              = archer_properties.sprite_path,
      velocity_walk_max   = archer_properties.maximum_walk_speed,
      acceleration_ground = archer_properties.ground_acceleration,
   })
end

function update(delta_time)
   -- sprite_time_s drives frame selection; state machine time drives timing guards.
   archer_visual_state.sprite_time_s = archer_visual_state.sprite_time_s + delta_time
   archer_visual_state.animation_time_s = archer_visual_state.animation_time_s + delta_time

   local previous_state = archer_state_machine.current_state_id
   archer_state_machine:advance_time(delta_time)
   local changed = (previous_state ~= archer_state_machine.current_state_id)

   update_sprite_frame(changed)
   updateKeysPressed(archer_visual_state.key_pressed_mask)
end

function setPath(name, table_kv)
   -- this hot-plugs patrol paths; if we were idle and a path appears, start walking immediately.
   if name == "path" then
      patrol_behavior:load_from_kv_table(table_kv)
      if archer_state_machine.current_state_id == state_id.idle and #patrol_behavior.waypoints > 0 then
         archer_state_machine:enter_state(state_id.walk)
         update_sprite_frame(true)
      end
   end
end
