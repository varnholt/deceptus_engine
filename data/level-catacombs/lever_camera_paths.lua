-- lever-triggered camera pans that show the effect of a lever before it gets used again
--
-- lever_spike_cannon shows a single on/off block column when first pulled, then toggles it.
-- lever_spike_01 shows three separate on/off block sets in sequence when first pulled, toggling
-- each set exactly when the camera reaches it, then pans back to where the camera was before the
-- sequence started. require this from the catacombs level script:
--
--    local lever_camera_paths = require "data/level-catacombs/lever_camera_paths"
--
--    function update(dt)
--       if (not _initialized) then
--          lever_camera_paths.init()
--       end
--       lever_camera_paths.update(dt)
--       cutscene.update(dt)
--    end
--
--    function mechanismEvent(object_id, group_id, event_name, value)
--       lever_camera_paths.mechanismEvent(object_id, event_name)
--    end
--
-- init() must run after the engine's mechanism lookup is wired up, so it belongs in the level
-- script's first update tick rather than in initialize(). cutscene.lua keeps a single shared event
-- queue, so init() combines both levers' sequences into one cutscene.load() call - a second call
-- would wipe out the first.

local cutscene = require "data/scripts/cutscene"

local LeverCameraPaths = {}

local _elapsed = 0.0

-- lever_spike_cannon: single on/off block column -------------------------------------------------

local _pixels_per_tile = 24
local _lever_spike_camera_x_offset_tiles = 11
local _lever_spike_camera_duration_s = 1.5
local _lever_spike_camera_hold_s = 2.0
local _lever_spike_camera_return_s = 1.0

-- the player is held in place until the camera is back on him, otherwise he can walk off the ledge
-- while the camera is showing the blocks somewhere else
local _lever_spike_player_lock_duration_s =
   _lever_spike_camera_duration_s + _lever_spike_camera_hold_s + _lever_spike_camera_return_s

local _on_off_block_ids = {
   "ct-on-off-block-01", "ct-on-off-block-02", "ct-on-off-block-03",
   "ct-on-off-block-04", "ct-on-off-block-05", "ct-on-off-block-06",
   "ct-on-off-block-07", "ct-on-off-block-08", "ct-on-off-block-09",
   "ct-on-off-block-10", "ct-on-off-block-11"
}
local _on_off_blocks_enabled = true
local _lever_spike_sequence_done = false
local _disable_on_off_blocks_at = -1.0

local function setOnOffBlocksEnabled(enabled)
   log(string.format("setOnOffBlocksEnabled: %s", tostring(enabled)))
   for _, block_id in ipairs(_on_off_block_ids) do
      setMechanismEnabled(block_id, enabled, "on_off_blocks")
   end
   _on_off_blocks_enabled = enabled
end

local function buildLeverSpikeActions()
   local spike_rect = getMechanismRect("ct-on-off-block-01")
   if (spike_rect) then
      log(string.format("bottom spike rect: x=%.0f y=%.0f", spike_rect.x, spike_rect.y))
      return {
         {
            on = "lever_spike_cannon_picked_up",
            action = "lock_player_controls",
            duration_s = _lever_spike_player_lock_duration_s
         },
         {
            on = "lever_spike_cannon_picked_up",
            action = "move_camera",
            x = spike_rect.x + _lever_spike_camera_x_offset_tiles * _pixels_per_tile,
            y = spike_rect.y + spike_rect.height * 0.5,
            duration_s = _lever_spike_camera_duration_s,
            easing = "ease_in_out",
            event = "camera_at_off_blocks"
         },
         {
            on = "camera_at_off_blocks",
            delay = _lever_spike_camera_hold_s,
            action = "unlock_camera"
         }
      }
   else
      log("ct-on-off-block column not found, camera pan will not work")
      return {}
   end
end

local function updateLeverSpike()
   if (_disable_on_off_blocks_at >= 0.0 and _elapsed >= _disable_on_off_blocks_at) then
      _disable_on_off_blocks_at = -1.0
      setOnOffBlocksEnabled(false)
   end
end

local function onLeverSpikeEvent()
   if (not _lever_spike_sequence_done) then
      _lever_spike_sequence_done = true
      _disable_on_off_blocks_at = _elapsed + _lever_spike_camera_duration_s + 0.5
      cutscene.notify("lever_spike_cannon_picked_up")
   else
      _on_off_blocks_enabled = not _on_off_blocks_enabled
      setOnOffBlocksEnabled(_on_off_blocks_enabled)
   end
end


-- lever_spike_01: three crossroads on/off block sets ----------------------------------------------

-- the blocks are enabled from this script rather than through the lever's target_ids, so each set
-- can be flipped exactly when the camera reaches it instead of all three flipping at once, before
-- the camera even starts moving
local _crossroads_group_1_ids = {"ct-crossroads-block9", "ct-crossroads-block10", "ct-crossroads-block11"}
local _crossroads_group_2_ids = {"ct-crossroads-block12", "ct-crossroads-block13", "ct-crossroads-block14"}
local _crossroads_group_3_ids = {"ct-crossroads-block1", "ct-crossroads-block2", "ct-crossroads-block3", "ct-crossroads-block4"}
local _crossroads_blocks_enabled = true

local _crossroads_toggle_group_1_at = -1.0
local _crossroads_toggle_group_2_at = -1.0
local _crossroads_toggle_group_3_at = -1.0

local _crossroads_group_1_rect = nil
local _crossroads_group_2_rect = nil
local _crossroads_group_3_rect = nil

local _lever_spike_01_camera_duration_s = 1.2
local _lever_spike_01_camera_hold_s = 1.5
local _lever_spike_01_camera_return_s = 1.0

local _lever_spike_01_player_lock_duration_s =
   _lever_spike_01_camera_duration_s * 3 + _lever_spike_01_camera_hold_s * 3 + _lever_spike_01_camera_return_s

local _lever_spike_01_sequence_done = false

local _lever_spike_01_group_1_action = nil
local _lever_spike_01_group_2_action = nil
local _lever_spike_01_group_3_action = nil
local _lever_spike_01_return_action = nil

local function setCrossroadsGroupEnabled(block_ids, enabled)
   for _, block_id in ipairs(block_ids) do
      setMechanismEnabled(block_id, enabled, "on_off_blocks")
   end
end

local function initLeverSpike01()
   setCrossroadsGroupEnabled(_crossroads_group_1_ids, _crossroads_blocks_enabled)
   setCrossroadsGroupEnabled(_crossroads_group_2_ids, _crossroads_blocks_enabled)
   setCrossroadsGroupEnabled(_crossroads_group_3_ids, _crossroads_blocks_enabled)
end

local function buildLeverSpike01Actions()
   _crossroads_group_1_rect = getMechanismRect("ct-crossroads-block10")
   _crossroads_group_2_rect = getMechanismRect("ct-crossroads-block13")
   _crossroads_group_3_rect = getMechanismRect("ct-crossroads-block3")

   if (_crossroads_group_1_rect and _crossroads_group_2_rect and _crossroads_group_3_rect) then
      -- the x/y fields below are placeholders. clampCameraToRoom only knows the correct room once
      -- the player is actually standing at the lever, so onLeverSpike01Event fills them in right
      -- before firing this sequence, keeping the whole view inside the room the lever sits in
      _lever_spike_01_group_1_action = {
         on = "lever_spike_01_pulled",
         action = "move_camera",
         x = 0,
         y = 0,
         duration_s = _lever_spike_01_camera_duration_s,
         easing = "ease_in_out",
         event = "camera_at_crossroads_group_1"
      }
      _lever_spike_01_group_2_action = {
         on = "camera_at_crossroads_group_1",
         delay = _lever_spike_01_camera_hold_s,
         action = "move_camera",
         x = 0,
         y = 0,
         duration_s = _lever_spike_01_camera_duration_s,
         easing = "ease_in_out",
         event = "camera_at_crossroads_group_2"
      }
      _lever_spike_01_group_3_action = {
         on = "camera_at_crossroads_group_2",
         delay = _lever_spike_01_camera_hold_s,
         action = "move_camera",
         x = 0,
         y = 0,
         duration_s = _lever_spike_01_camera_duration_s,
         easing = "ease_in_out",
         event = "camera_at_crossroads_group_3"
      }
      _lever_spike_01_return_action = {
         on = "camera_at_crossroads_group_3",
         delay = _lever_spike_01_camera_hold_s,
         action = "move_camera",
         x = 0,
         y = 0,
         duration_s = _lever_spike_01_camera_return_s,
         easing = "ease_in_out",
         event = "camera_back_at_lever_spike_01"
      }

      return {
         {
            on = "lever_spike_01_pulled",
            action = "lock_player_controls",
            duration_s = _lever_spike_01_player_lock_duration_s
         },
         _lever_spike_01_group_1_action,
         _lever_spike_01_group_2_action,
         _lever_spike_01_group_3_action,
         _lever_spike_01_return_action,
         {
            on = "camera_back_at_lever_spike_01",
            action = "unlock_camera"
         }
      }
   else
      log("ct-crossroads-block group not found, camera pan will not work")
      return {}
   end
end

local function updateLeverSpike01()
   if (_crossroads_toggle_group_1_at >= 0.0 and _elapsed >= _crossroads_toggle_group_1_at) then
      _crossroads_toggle_group_1_at = -1.0
      setCrossroadsGroupEnabled(_crossroads_group_1_ids, _crossroads_blocks_enabled)
   end

   if (_crossroads_toggle_group_2_at >= 0.0 and _elapsed >= _crossroads_toggle_group_2_at) then
      _crossroads_toggle_group_2_at = -1.0
      setCrossroadsGroupEnabled(_crossroads_group_2_ids, _crossroads_blocks_enabled)
   end

   if (_crossroads_toggle_group_3_at >= 0.0 and _elapsed >= _crossroads_toggle_group_3_at) then
      _crossroads_toggle_group_3_at = -1.0
      setCrossroadsGroupEnabled(_crossroads_group_3_ids, _crossroads_blocks_enabled)
   end
end

local function onLeverSpike01Event()
   _crossroads_blocks_enabled = not _crossroads_blocks_enabled

   if (not _lever_spike_01_sequence_done) then
      _lever_spike_01_sequence_done = true

      -- clamp against the room now, with the player actually standing at the lever, so the
      -- correct room boundary is in effect - clamping is meaningless before the player gets there
      local crossroads_group_1_camera = clampCameraToRoom(
         _crossroads_group_1_rect.x + _crossroads_group_1_rect.width * 0.5,
         _crossroads_group_1_rect.y + _crossroads_group_1_rect.height * 0.5
      )
      local crossroads_group_2_camera = clampCameraToRoom(
         _crossroads_group_2_rect.x + _crossroads_group_2_rect.width * 0.5,
         _crossroads_group_2_rect.y + _crossroads_group_2_rect.height * 0.5
      )
      local crossroads_group_3_camera = clampCameraToRoom(
         _crossroads_group_3_rect.x + _crossroads_group_3_rect.width * 0.5,
         _crossroads_group_3_rect.y + _crossroads_group_3_rect.height * 0.5
      )

      _lever_spike_01_group_1_action.x = crossroads_group_1_camera.x
      _lever_spike_01_group_1_action.y = crossroads_group_1_camera.y
      _lever_spike_01_group_2_action.x = crossroads_group_2_camera.x
      _lever_spike_01_group_2_action.y = crossroads_group_2_camera.y
      _lever_spike_01_group_3_action.x = crossroads_group_3_camera.x
      _lever_spike_01_group_3_action.y = crossroads_group_3_camera.y

      -- pan back to wherever the player's camera actually rests, so handing tracking back to the
      -- engine on unlock_camera doesn't produce a small visible correction. getCameraCenter() would
      -- return the camera's live position, which can still be a few pixels short of the true rest
      -- point without that gap ever having looked like motion (see getRestingCameraCenter's doc)
      local camera_center = getRestingCameraCenter()
      _lever_spike_01_return_action.x = camera_center.x
      _lever_spike_01_return_action.y = camera_center.y

      -- toggle each block set exactly when the camera arrives there, instead of all at once
      local arrival_group_1_s = _lever_spike_01_camera_duration_s
      local arrival_group_2_s = arrival_group_1_s + _lever_spike_01_camera_hold_s + _lever_spike_01_camera_duration_s
      local arrival_group_3_s = arrival_group_2_s + _lever_spike_01_camera_hold_s + _lever_spike_01_camera_duration_s

      _crossroads_toggle_group_1_at = _elapsed + arrival_group_1_s
      _crossroads_toggle_group_2_at = _elapsed + arrival_group_2_s
      _crossroads_toggle_group_3_at = _elapsed + arrival_group_3_s

      cutscene.notify("lever_spike_01_pulled")
   else
      setCrossroadsGroupEnabled(_crossroads_group_1_ids, _crossroads_blocks_enabled)
      setCrossroadsGroupEnabled(_crossroads_group_2_ids, _crossroads_blocks_enabled)
      setCrossroadsGroupEnabled(_crossroads_group_3_ids, _crossroads_blocks_enabled)
   end
end


------------------------------------------------------------------------------------------------------------------------
function LeverCameraPaths.init()
   setOnOffBlocksEnabled(true)
   initLeverSpike01()

   local actions = {}
   for _, entry in ipairs(buildLeverSpikeActions()) do
      table.insert(actions, entry)
   end
   for _, entry in ipairs(buildLeverSpike01Actions()) do
      table.insert(actions, entry)
   end
   cutscene.load(actions)
end


------------------------------------------------------------------------------------------------------------------------
function LeverCameraPaths.update(dt)
   _elapsed = _elapsed + dt
   updateLeverSpike()
   updateLeverSpike01()
end


------------------------------------------------------------------------------------------------------------------------
function LeverCameraPaths.mechanismEvent(object_id, event_name)
   -- pan camera to the on/off blocks on first lever_spike_cannon toggle, then toggle immediately
   if (object_id == "lever_spike_cannon" and event_name == "state") then
      onLeverSpikeEvent()
   end

   -- pan camera through the three crossroads on/off block sets the first time lever_spike_01 is
   -- pulled, toggling each set exactly when the camera reaches it
   if (object_id == "lever_spike_01" and event_name == "state") then
      onLeverSpike01Event()
   end
end

return LeverCameraPaths
