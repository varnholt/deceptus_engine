-- shared cutscene runner
-- loads a table of actions (returned by loadCutscene), dispatches time-based
-- and event-based entries. require this from any level script that runs cutscenes.

local Cutscene = {}

local _music_transition = {
   let_current_finish = 0,
   crossfade          = 1,
   immediate          = 2,
   fade_out_then_new  = 3,
}

local _music_post_action = {
   none      = 0,
   loop      = 1,
   play_next = 2,
}

local _elapsed         = 0.0
local _time_actions    = {}
local _event_queues    = {}   -- event_name -> list of groups; each group = {entry, ...}
local _pending_delayed = {}   -- list of {fire_at, entry}
local _active          = false

local function execute(entry)
   local action = entry.action
   if action == "set_camera_position" then
      setCameraPosition(entry.x, entry.y)
   elseif action == "play_music" then
      playMusic(
         entry.file,
         _music_transition[entry.transition] or 0,
         entry.duration_ms or 0,
         _music_post_action[entry.post_action] or 0
      )
   elseif action == "play_sound" then
      playSound(entry.id)
   elseif action == "create_sprite" then
      createSprite(entry.name, entry.animation_file, entry.animation, entry.x, entry.y)
   elseif action == "destroy_sprite" then
      destroySprite(entry.name)
   elseif action == "set_sprite_animation" then
      setSpriteAnimation(entry.name, entry.animation)
   elseif action == "set_sprite_visible" then
      setSpriteVisible(entry.name, entry.visible)
   elseif action == "move_sprite" then
      moveSpriteAtSpeed(entry.name, entry.x, entry.y, entry.speed, entry.event)
   elseif action == "fade_in" then
      fadeIn(entry.speed)
   elseif action == "fade_out" then
      fadeOut(entry.speed)
   elseif action == "show_dialogue" then
      showDialogue(entry.id)
   elseif action == "set_player_visible" then
      setPlayerVisible(entry.visible)
   elseif action == "set_hud_visible" then
      setHudVisible(entry.visible)
   elseif action == "unlock_camera" then
      unlockCamera()
   elseif action == "next_level" then
      nextLevel()
   end
end

local function schedule(entry)
   if entry.delay and entry.delay > 0.0 then
      table.insert(_pending_delayed, {fire_at = _elapsed + entry.delay, entry = entry})
   else
      execute(entry)
   end
end

-- consecutive entries sharing the same 'on' value form one group; each
-- group fires on one occurrence of the event and is then consumed.
local function build_event_queues(actions)
   _event_queues = {}
   local current_event = nil
   local current_group = nil
   for _, entry in ipairs(actions) do
      if entry.on then
         if entry.on ~= current_event then
            current_event = entry.on
            current_group = {}
            if not _event_queues[current_event] then
               _event_queues[current_event] = {}
            end
            table.insert(_event_queues[current_event], current_group)
         end
         table.insert(current_group, entry)
      else
         current_event = nil
         current_group = nil
      end
   end
end

function Cutscene.load(actions)
   _elapsed         = 0.0
   _pending_delayed = {}
   _active          = true

   _time_actions = {}
   for _, entry in ipairs(actions) do
      if entry.at then
         entry._fired = false
         table.insert(_time_actions, entry)
      end
   end

   build_event_queues(actions)
end

function Cutscene.update(dt)
   if not _active then
      return
   end

   _elapsed = _elapsed + dt

   for _, entry in ipairs(_time_actions) do
      if not entry._fired and _elapsed >= entry.at then
         entry._fired = true
         schedule(entry)
      end
   end

   local still_pending = {}
   for _, pending in ipairs(_pending_delayed) do
      if _elapsed >= pending.fire_at then
         execute(pending.entry)
      else
         table.insert(still_pending, pending)
      end
   end
   _pending_delayed = still_pending
end

function Cutscene.notify(event_name)
   if not _active then
      return
   end
   local queue = _event_queues[event_name]
   if not queue or #queue == 0 then
      return
   end
   local group = table.remove(queue, 1)
   for _, entry in ipairs(group) do
      schedule(entry)
   end
end

function Cutscene.stop()
   _active = false
end

return Cutscene
