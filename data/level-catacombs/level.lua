require "data/level-catacombs/level_constants"
local cutscene = require "data/scripts/cutscene"
local music_zones = require "data/scripts/music_zones"

------------------------------------------------------------------------------------------------------------------------

_initialized = false

_elapsed = 0.0

_delay_to_show_dialogue = 1.5
_delay_to_start_kneel = 1.0
_delay_to_show_monk = 0.5
_delay_to_hide_monk = 2.0

_player_intersected_with_monk_rect = false
_monk_shown = false
_player_kneeled = false
_monk_dialogue_shown = false
_monk_hide = false

_delay_to_show_dive_dialogue = 1.0
_player_intersected_with_water_block_sensor = false
_player_wont_dive_dialogue_shown = false

-- the rubies the player takes from the owl in the graveyard
_owl_eye_item = "gems"

-- inserting them is remembered in the save game, the inventory cannot say it: the gems are gone
-- afterwards, which looks the same as never having had them
_owl_eyes_inserted_treasure = "catacombs_owl_eyes_inserted"

_sword_cage_ids = {"sword_cage_01", "sword_cage_02", "sword_cage_03", "sword_cage_04"}
_shrine_inserted_last = nil
_shrine_carrying_last = nil

-- the rubies seat with a sound, and the ward only lets go once that has landed
_shrine_insert_sample = "mechanism_switch_lever_insert.ogg"
_shrine_release_delay_s = 0.45
_release_sword_ring_at = -1.0

_sword_pickup_flash_color = {r = 255, g = 244, b = 214}
_sword_pickup_flash_intensity = 0.55
_sword_pickup_flash_duration_s = 0.45

_pixels_per_tile = 24
_lever_spike_camera_x_offset_tiles = 11
_lever_spike_camera_duration_s = 1.5
_lever_spike_camera_hold_s = 2.0
_lever_spike_camera_return_s = 1.0

-- the player is held in place until the camera is back on him, otherwise he can walk off the ledge
-- while the camera is showing the blocks somewhere else
_lever_spike_player_lock_duration_s =
   _lever_spike_camera_duration_s + _lever_spike_camera_hold_s + _lever_spike_camera_return_s

_on_off_block_ids = {
   "ct-on-off-block-01", "ct-on-off-block-02", "ct-on-off-block-03",
   "ct-on-off-block-04", "ct-on-off-block-05", "ct-on-off-block-06",
   "ct-on-off-block-07", "ct-on-off-block-08", "ct-on-off-block-09",
   "ct-on-off-block-10", "ct-on-off-block-11"
}
_on_off_blocks_enabled = true
_lever_spike_sequence_done = false
_disable_on_off_blocks_at = -1.0


------------------------------------------------------------------------------------------------------------------------
function setOnOffBlocksEnabled(enabled)
   log(string.format("setOnOffBlocksEnabled: %s", tostring(enabled)))
   for _, block_id in ipairs(_on_off_block_ids) do
      setMechanismEnabled(block_id, enabled, "on_off_blocks")
   end
   _on_off_blocks_enabled = enabled
end


------------------------------------------------------------------------------------------------------------------------
function setSwordCageEnabled(enabled)
   for _, cage_id in ipairs(_sword_cage_ids) do
      setMechanismEnabled(cage_id, enabled, "blocking_rects")
   end
end


------------------------------------------------------------------------------------------------------------------------
-- the owl is drawn with hollow sockets in the decoration-b tiles; the "owl-eyes" image layer is
-- what paints the rubies into them, so showing the eyes is all it takes to fill the sockets.
--
--   owl-eyes image layer   ->  rubies in the sockets, ring dead, cage open
--   decoration-b tiles     ->  the same owl with empty sockets, ring alive, sword caged
--
function setOwlEyesInserted(inserted)
   setMechanismVisible("owl-eyes", inserted, "imagelayers")
   setMechanismEnabled("shrine_dialogue", not inserted, "dialogues")
   releaseSwordRing(inserted)
end


------------------------------------------------------------------------------------------------------------------------
-- the ring expanding away and the cage opening are the same moment: the ward stops holding on
function releaseSwordRing(released)
   setMechanismEnabled("sword_ring", not released, "shader_quads")
   setSwordCageEnabled(not released)
end


------------------------------------------------------------------------------------------------------------------------
function updateShrineRelease()
   if (_release_sword_ring_at >= 0.0 and _elapsed >= _release_sword_ring_at) then
      _release_sword_ring_at = -1.0
      releaseSwordRing(true)
   end
end


------------------------------------------------------------------------------------------------------------------------
-- which prompt the shrine offers depends on whether the player is carrying the rubies
function updateShrineHelp(inserted)
   local can_insert = (not inserted) and inventoryHas(_owl_eye_item)
   setMechanismEnabled("shrine_help_insert", can_insert, "interaction_help")
   setMechanismEnabled("shrine_help_examine", not can_insert and not inserted, "interaction_help")
end


------------------------------------------------------------------------------------------------------------------------
function initShrine()
   local inserted = hasTreasure(_owl_eyes_inserted_treasure)
   setOwlEyesInserted(inserted)
   updateShrineHelp(inserted)
end


------------------------------------------------------------------------------------------------------------------------
function insertOwlEyes()
   if (hasTreasure(_owl_eyes_inserted_treasure)) then
      return
   end

   if (not inventoryHas(_owl_eye_item)) then
      return
   end

   addTreasure(_owl_eyes_inserted_treasure)
   inventoryRemove(_owl_eye_item)

   setMechanismVisible("owl-eyes", true, "imagelayers")
   setMechanismEnabled("shrine_dialogue", false, "dialogues")
   updateShrineHelp(true)

   playSound(_shrine_insert_sample)
   _release_sword_ring_at = _elapsed + _shrine_release_delay_s
end


------------------------------------------------------------------------------------------------------------------------
function initLeverSpike()
   setOnOffBlocksEnabled(true)
   local spike_rect = getMechanismRect("ct-on-off-block-01")
   if (spike_rect) then
      log(string.format("bottom spike rect: x=%.0f y=%.0f", spike_rect.x, spike_rect.y))
      cutscene.load({
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
      })
   else
      log("ct-on-off-block column not found, camera pan will not work")
   end
end


------------------------------------------------------------------------------------------------------------------------
function updateLeverSpike()
   if (_disable_on_off_blocks_at >= 0.0 and _elapsed >= _disable_on_off_blocks_at) then
      _disable_on_off_blocks_at = -1.0
      setOnOffBlocksEnabled(false)
   end
end


------------------------------------------------------------------------------------------------------------------------
function onLeverSpikeEvent()
   if (not _lever_spike_sequence_done) then
      _lever_spike_sequence_done = true
      _disable_on_off_blocks_at = _elapsed + _lever_spike_camera_duration_s + 0.5
      cutscene.notify("lever_spike_cannon_picked_up")
   else
      _on_off_blocks_enabled = not _on_off_blocks_enabled
      setOnOffBlocksEnabled(_on_off_blocks_enabled)
   end
end


------------------------------------------------------------------------------------------------------------------------
function initialize()
   -- log("initialize catacombs level script")
   setInfoLayerVisible(true)

   -- the sewers get their own track. checkpoint 1 sits behind the sewers entrance, so from there
   -- on the sewers track is what the level starts with - also after dying or reloading the level.
   music_zones.configure({
      default_track = "data/music/level_test_track_muffler_awakening.ogg",
      zones = {
         {
            sensor_rect = "zone_rect",
            track = "data/music/level_test_track_muffler_ancestors.ogg",
            from_checkpoint = 1
         }
      }
   })
end


------------------------------------------------------------------------------------------------------------------------
function writeProperty(key, value)
   -- log(string.format("write property: %s %s", key, value))
end


------------------------------------------------------------------------------------------------------------------------
function makeMonkVisible(dt)

   _delay_to_show_monk = _delay_to_show_monk - dt
   
   if (_delay_to_show_monk <= 0.0) then
      writeLuaNodeProperty("shadow", "show", "true")
      setLuaNodeVisible("shadow", true)
      _monk_shown = true
      setZoomFactor(0.75)
      lockPlayerControls(5000)
   end
end


------------------------------------------------------------------------------------------------------------------------
function makePlayerKneel(dt)
   _delay_to_start_kneel = _delay_to_start_kneel - dt
   if (_delay_to_start_kneel <= 0.0) then
      playEventRecording("data/level-catacombs/animation_player_kneel.dat")
      _player_kneeled = true
   end
end


------------------------------------------------------------------------------------------------------------------------
function makeMonkSpeak(dt)

   _delay_to_show_dialogue = _delay_to_show_dialogue - dt

   if (_delay_to_show_dialogue <= 0.0) then
      log("make monk speak")
      _monk_dialogue_shown = true
      showDialogue("monk")
   end
end


------------------------------------------------------------------------------------------------------------------------
function makeMonkDisappear(dt)

   _delay_to_hide_monk = _delay_to_hide_monk - dt

   if (_delay_to_hide_monk <= 0.0) then
      log("make monk disappear")
      _monk_hide = false
      writeLuaNodeProperty("shadow", "hide", "true")
      setZoomFactor(1.0)
   end
end


------------------------------------------------------------------------------------------------------------------------
function updateSwimAllowed(dt)
   if (_player_intersected_with_water_block_sensor) then
      _delay_to_show_dive_dialogue = _delay_to_show_dive_dialogue - dt
      if (not _player_wont_dive_dialogue_shown and _delay_to_show_dive_dialogue <= 0.0) then
         _player_wont_dive_dialogue_shown = true
         showDialogue("cant_swim_here_01")
      end
   end
end


------------------------------------------------------------------------------------------------------------------------
function openLocker()
   log("open locker")
   setMechanismVisible("locker_open", true, "imagelayers")
   setMechanismVisible("handle", true, "extras")
   setMechanismEnabled("handle", true, "extras")
   setMechanismEnabled("locker_dialogue_locked", false, "dialogues")
   setMechanismEnabled("locker_dialogue_unlocked", true, "dialogues")
end


------------------------------------------------------------------------------------------------------------------------
function initLocker()
   setMechanismVisible("locker_open", false, "imagelayers")
   setMechanismVisible("handle", false, "extras")
   setMechanismEnabled("handle", false, "extras")
   setMechanismEnabled("locker_dialogue_locked", true, "dialogues")
   setMechanismEnabled("locker_dialogue_unlocked", false, "dialogues")
end


------------------------------------------------------------------------------------------------------------------------
function initDrawer()
   if (inventoryHas("solar_seal")) then
      setMechanismVisible("drawer_open", true, "imagelayers")
      setMechanismEnabled("drawer_rect", false, "button_rects")
      setMechanismEnabled("drawer_interaction_help", false, "interaction_help")
      setMechanismEnabled("drawer_dialogue_inspect", false, "dialogues")
      setMechanismEnabled("drawer_dialogue_key", false, "dialogues")
   else
      setMechanismVisible("drawer_open", false, "imagelayers")
   end
end

------------------------------------------------------------------------------------------------------------------------
function initLockedBox()
   -- the chest restores its own open state from the save game, so the texts that describe it are derived from
   -- the chest instead of being remembered separately
   -- the search group is the tmx layer name, which is not the group id that mechanismEvent reports
   if (getMechanismProperty("locked_box", "treasure_chests", "open")) then
      setMechanismEnabled("locked_message", false, "dialogues")
      setMechanismEnabled("locked_box_interaction_help", false, "interaction_help")
      setMechanismEnabled("handle_help", false, "interaction_help")
   end
end


------------------------------------------------------------------------------------------------------------------------
function openDrawer()
   if (inventoryHas("solar_seal")) then
      return
   end
   log("open drawer")
   setMechanismVisible("drawer_open", true, "imagelayers")
   setMechanismEnabled("drawer_rect", false, "button_rects")
   setMechanismEnabled("drawer_interaction_help", false, "interaction_help")
   setMechanismEnabled("drawer_dialogue_inspect", false, "dialogues")
   showDialogue("drawer_dialogue_key")
   setMechanismEnabled("drawer_dialogue_key", false, "dialogues")
   inventoryAdd("solar_seal")
end


------------------------------------------------------------------------------------------------------------------------
function updateMonk(dt)

   -- monk dialogue logic
   if (not _player_intersected_with_monk_rect) then
      return
   end
      
   -- 1. make him visible
   -- 2. make him speak
   -- 3. make him disappear again
   if (not _monk_shown) then
      makeMonkVisible(dt)
   elseif (not _player_kneeled) then
      makePlayerKneel(dt)
   elseif (not _monk_dialogue_shown) then
      makeMonkSpeak(dt)
   elseif (_monk_hide) then
      makeMonkDisappear(dt)
   end
end


------------------------------------------------------------------------------------------------------------------------
-- the player can equip or drop the rubies while standing at the shrine, and there is no callback
-- for the inventory changing, so the prompt is re-derived every frame
function updateShrine()
   local inserted = hasTreasure(_owl_eyes_inserted_treasure)
   if (inserted ~= _shrine_inserted_last or inventoryHas(_owl_eye_item) ~= _shrine_carrying_last) then
      _shrine_inserted_last = inserted
      _shrine_carrying_last = inventoryHas(_owl_eye_item)
      updateShrineHelp(inserted)
   end
end


------------------------------------------------------------------------------------------------------------------------
function update(dt)
   _elapsed = _elapsed + dt

   -- initialization
   if (not _initialized) then
      _initialized = true
      
      -- init monk
      addSensorRectCallback("monk_rect")
      setLuaNodeVisible("shadow", false)
      
      -- underwater barrier
      addSensorRectCallback("water_block_sensor_01")

      -- change music on zone enter event
      music_zones.registerSensorRects()

      addSensorRectCallback("sword_ring_sensor")

      initShrine()
      initLocker()
      initDrawer()
      initLockedBox()
      initLeverSpike()
   end

   updateLeverSpike()
   updateShrine()
   updateShrineRelease()

   updateMonk(dt)
   updateSwimAllowed(dt)
   cutscene.update(dt)
end


------------------------------------------------------------------------------------------------------------------------
function mechanismEvent(object_id, group_id, event_name, value)
   log(string.format("mechanismEvent: id='%s' group='%s' event='%s' value='%s'", object_id, group_id, event_name, tostring(value)))

   -- player pressed the action button in front of the shrine
   if (object_id == "shrine_rect" and event_name == "pressed" and value == "true") then
      insertOwlEyes()
   end

   -- update door dialogue when open
   if (object_id == "iron_door" and event_name == "state" and value == "opening") then
      setMechanismEnabled("door_locked_dialogue", false, "dialogues")
      setMechanismEnabled("door_opened_dialogue", true, "dialogues")
   end
   
   -- dialogue with monk done, hide him
   if (object_id == "monk" and event_name == "state" and value == "hide") then
      log("hide monk")
      _monk_hide = true
   end

   -- handle inserted into lever_cell, hide the "lever is missing" dialogue and examine hint
   if (object_id == "lever_cell" and event_name == "handle_inserted") then
      setMechanismEnabled("lever_cell_dialogue", false, "dialogues")
      setMechanismEnabled("lever_cell_help", false, "interaction_help")
   end

   -- pan camera to the on/off blocks on first lever_spike_cannon toggle, then toggle immediately
   if (object_id == "lever_spike_cannon" and event_name == "state") then
      onLeverSpikeEvent()
   end

   -- treasure chest is locked
   if (object_id == "locked_box" and event_name == "state" and value == "locked") then
      showDialogue("locked_message")
   end
   
   -- open treasure chest
   if (object_id == "locked_box" and event_name == "state" and value == "open") then
      setMechanismEnabled("locked_message", false, "dialogues")
      setMechanismEnabled("locked_box_interaction_help", false, "interaction_help")
      setMechanismEnabled("handle_help", false, "interaction_help")
   end

   -- open diamond box
   if (object_id == "diamond_box" and event_name == "state" and value == "open") then
      setMechanismEnabled("diamond_box_interaction_help", false, "interaction_help")
   end

   -- the sealed cells are pitch black, so the tunnel stays blocked while the head torch is off.
   -- group_id is checked because the extra the locked box spawns is called "headtorch" as well.
   if (object_id == "headtorch" and group_id == "items" and event_name == "state") then
      local torch_is_off = (value == "off")
      setMechanismEnabled("tunnel_block", torch_is_off, "blocking_rects")
      setMechanismEnabled("cant_see_here_01", torch_is_off, "dialogues")
   end
   
   -- open drawer in library
   if (object_id == "drawer_rect" and event_name == "pressed" and value == "true") then
      openDrawer()
   end
end


------------------------------------------------------------------------------------------------------------------------
function playerReceivedItem(item)

   log(string.format("received item: %s", item))

end


------------------------------------------------------------------------------------------------------------------------
function playerUsedItem(item)

   log(string.format("used item: %s", item))

   -- when sensor rect flag set and key is used on locker, then change
   if (item == "locker_key") then
      local player_in_front_of_locker = isPlayerIntersectingSensorRect("locker_sensor")
      log(player_in_front_of_locker and "true" or "false")
      if (player_in_front_of_locker) then
         openLocker()
         inventoryRemove("locker_key")
      end
      return true
   end

   return false
end



------------------------------------------------------------------------------------------------------------------------
function playerReceivedExtra(extra)
   -- log(string.format("player received extra: %s", extra))
   
   -- enable all blocking rects once player picked up diving suit
   if (extra == "sword") then
      giveWeaponSword()
      flashScreen(
         _sword_pickup_flash_color.r,
         _sword_pickup_flash_color.g,
         _sword_pickup_flash_color.b,
         _sword_pickup_flash_intensity,
         _sword_pickup_flash_duration_s
      )
   end
   
   if (extra == "handle") then
      showDialogue("handle_dialogue_2")
      setMechanismEnabled("handle_help", false, "interaction_help")
      setMechanismEnabled("handle_dialogue_1", false, "dialogues")
      setMechanismEnabled("locker_dialogue_unlocked", false, "dialogues")
   end

   if (extra == "locker_key") then
      -- change text when inspecting locker "use key"
      showDialogue("locker_key_acquired")
      setMechanismEnabled("ih_locker_key", false, "interaction_help")
      setMechanismEnabled("d_locker_key", false, "dialogues")
   end
   
   if (extra == "solar_seal") then
      setMechanismEnabled("drawer_dialogue_key", false, "dialogues")
   end

   -- head torch spawned by the locked box; Extra already put it in the
   -- inventory, this is just the acquired message
   if (extra == "headtorch") then
      showDialogue("headtorch_acquired")
   end
   
   if extra:match("^heart_") then
      addPlayerHealthMax(1)
      addPlayerHealth(255)
   end

end


------------------------------------------------------------------------------------------------------------------------
function playerCollidesWithSensorRect(rect_id)
   log(string.format("sensor rect collision: %s", rect_id))
   
   if (music_zones.playerCollidesWithSensorRect(rect_id)) then
      return
   end

   if (rect_id == "monk_rect") then
      _player_intersected_with_monk_rect = true
   elseif (rect_id == "water_block_sensor_01") then
      _player_intersected_with_water_block_sensor = true
   elseif (rect_id == "sword_ring_sensor") then
      -- the ring reacts to the player on its own, from RingShaderLayer::updateTouch. this hook is
      -- kept for whatever wants to know he reached the ring.
      -- log("player entered the sword ring")
   end
end



-- [player in front of locker] -> [player presses use]
--                                         |
--                                     [locked?] -------------------------+
--                                         |                              |
--                                         | yes                          | no
--                                         |                              |
--                                     [has key?] ----------+        [lever present] -------------+
--                                         |                |             |                       |
--                                         | yes            | no          | yes                   | no
--                                         |                |             |                       |
--                                      [unlock]       [show text]   [player gets lever]     [no action]
--                                         |            "need key"
--                                         |
--                                         |
--                                      [remove key]
--                                         |
--                                         |
--                                         |
--                                      [show text]
--                                       "got lever!"
--                                       
-- 
-- 
-- [player in front of locker] -> [player presses examine]
--                                         |
--                                     [locked?] -------------------------+
--                                         |                              |
--                                         | yes                          | no
--                                         |                              |
--                                     [has key?] ----------+        [lever present] -------------+
--                                         |                |             |                       |
--                                         | yes            | no          | yes                   | no
--                                         |                |             |                       |
--                                     [show text]       [show text]   [show text]             [show text]
--                                     "suggest use"     "need key"    "lever inside!"         "it's empty"
                                                
