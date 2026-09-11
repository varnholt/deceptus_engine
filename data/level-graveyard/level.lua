------------------------------------------------------------------------------------------------------------------------

_initialized = false

-- the owl statue carries two cut rubies as eyes, and the inventory item that holds them is called "gems"
_owl_eye_item = "gems"

-- seconds left until the pickup message is shown, or nil when none is pending
_pickup_message_delay_s = nil

-- long enough for the strike to land and start decaying before the message slides in
_pickup_message_delay_default_s = 1.5


------------------------------------------------------------------------------------------------------------------------
function initialize()
   setInfoLayerVisible(true)
end


------------------------------------------------------------------------------------------------------------------------
-- the tile layers already draw the owl with hollow sockets, the "owl-eyes" image layer is what paints the two
-- rubies into them. taking the eyes therefore only has to hide that image layer.
--
--   owl-eyes image layer   ->  glowing rubies in the sockets
--   decoration-b tiles     ->  the same shrine with empty sockets
--
function setOwlEyesPresent(present)
   setMechanismVisible("owl-eyes", present, "imagelayers")
   setMechanismEnabled("shrine_rect", present, "button_rects")
   setMechanismEnabled("shrine_help_take", present, "interaction_help")
   setMechanismEnabled("shrine_help_examine", not present, "interaction_help")
end


------------------------------------------------------------------------------------------------------------------------
-- the birds own the graveyard for as long as the weather holds. once the storm breaks they give it up, so the two are
-- switched together: the emitter fades itself out over its fade_duration_s rather than cutting off mid call.
function setStormActive(active)
   setMechanismEnabled("thunderstorm", active, "weather")
   setMechanismEnabled("birds", not active, "sound_emitters")
end


------------------------------------------------------------------------------------------------------------------------
-- the rubies go into the inventory, and the inventory is part of the save game. that makes "the player carries the
-- owl's eyes" the persisted flag which decides whether the thunderstorm is already awake when the level is entered.
function initShrine()
   local owl_eyes_taken = inventoryHas(_owl_eye_item)
   setOwlEyesPresent(not owl_eyes_taken)
   setStormActive(owl_eyes_taken)
end


------------------------------------------------------------------------------------------------------------------------
function takeOwlEyes()
   if (inventoryHas(_owl_eye_item)) then
      return
   end

   log("owl eyes taken")

   setOwlEyesPresent(false)
   inventoryAdd(_owl_eye_item)

   -- the storm the owl was keeping asleep breaks the moment its eyes come out, on the same frame the
   -- sockets go dark. the strike is fired through the weather mechanism rather than as a screen flash
   -- so it looks and sounds like the storm that follows it
   setStormActive(true)
   strikeThunderMechanism("thunderstorm", 1.0, "weather")

   -- the pickup message is the same one every item shows, it just waits for the thunder
   _pickup_message_delay_s = _pickup_message_delay_default_s
end


------------------------------------------------------------------------------------------------------------------------
function update(dt)
   -- the mechanism lookup is not wired up yet while initialize() runs, so the shrine is set up on the first frame
   if (not _initialized) then
      _initialized = true
      initShrine()
   end

   if (_pickup_message_delay_s ~= nil) then
      _pickup_message_delay_s = _pickup_message_delay_s - dt
      if (_pickup_message_delay_s <= 0.0) then
         _pickup_message_delay_s = nil
         showDialogue("rubies_acquired")
      end
   end
end


------------------------------------------------------------------------------------------------------------------------
function mechanismEvent(object_id, group_id, event_name, value)
   -- player pressed the action button in front of the shrine
   if (object_id == "shrine_rect" and event_name == "pressed" and value == "true") then
      takeOwlEyes()
   end

end


------------------------------------------------------------------------------------------------------------------------
function playerCollidesWithRect(rect_id)
end


------------------------------------------------------------------------------------------------------------------------
function playerCollidesWithSensorRect(rect_id)
end
