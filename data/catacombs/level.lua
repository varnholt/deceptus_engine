require "data/catacombs/level_constants"

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
_player_intersected_with_zone_rect = false


------------------------------------------------------------------------------------------------------------------------
function initialize()
   -- log("initialize catacombs level script")
end


------------------------------------------------------------------------------------------------------------------------
function writeProperty(key, value)
   -- log(string.format("write property: %s %s", key, value))
end


------------------------------------------------------------------------------------------------------------------------
function makeMonkVisible(dt)

    log("make monk visible")

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
      playEventRecording("data/catacombs/animation_player_kneel.dat")
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
      addSensorRectCallback("zone_rect")

      initLocker()
   end

   updateMonk(dt) 
   updateSwimAllowed(dt)  
end


------------------------------------------------------------------------------------------------------------------------
function mechanismEvent(object_id, group_id, event_name, value)

   log(string.format("object_id: %s, group_id: %s, event_name: %s, value: %s", object_id, group_id, event_name, tostring(value)))

   if (object_id == "iron_door" and event_name == "state" and value == "opening") then
      setMechanismEnabled("door_locked_dialogue", false, "dialogues")
      setMechanismEnabled("door_opened_dialogue", true, "dialogues")
   end
   
   -- object_id: monk, group_id: dialogues, event_name: state, value: hide
   if (object_id == "monk" and event_name == "state" and value == "hide") then
      log("hide monk")
      _monk_hide = true
   end

   if (object_id == "locked_box" and event_name == "state" and value == "locked") then
      log("burp")
      showDialogue("locked_message")
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
   end
   
   if (extra == "handle") then
      showDialogue("handle_dialogue_2")
      setMechanismEnabled("handle_help", false, "interaction_help")
      setMechanismEnabled("handle_dialogue_1", false, "dialogues")
   end

   if (extra == "locker_key") then
      -- change text when inspecting locker "use key"
   end
   
   if extra:match("^heart_") then
      addPlayerHealthMax(1)
      addPlayerHealth(255)
   end

end


------------------------------------------------------------------------------------------------------------------------
function playerCollidesWithSensorRect(rect_id)
   log(string.format("sensor rect collision: %s", rect_id))
   
   if (rect_id == "monk_rect") then
      _player_intersected_with_monk_rect = true
   elseif (rect_id == "water_block_sensor_01") then
      _player_intersected_with_water_block_sensor = true
   elseif (rect_id == "zone_rect") then
      if (not _player_intersected_with_zone_rect) then
         _player_intersected_with_zone_rect = true
         playMusic("data/music/level_test_track_muffler_ancestors.ogg", MusicTransitionType.Crossfade, 1000, MusicPostPlaybackAction.None)
      end
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
                                                
