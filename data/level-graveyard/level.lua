------------------------------------------------------------------------------------------------------------------------

_initialized = false

-- the owl statue carries two cut rubies as eyes, and the inventory item that holds them is called "gems"
_owl_eye_item = "gems"


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
-- the rubies go into the inventory, and the inventory is part of the save game. that makes "the player carries the
-- owl's eyes" the persisted flag which decides whether the thunderstorm is already awake when the level is entered.
function initShrine()
   local owl_eyes_taken = inventoryHas(_owl_eye_item)
   setOwlEyesPresent(not owl_eyes_taken)
   setMechanismEnabled("thunderstorm", owl_eyes_taken, "weather")
end


------------------------------------------------------------------------------------------------------------------------
function takeOwlEyes()
   if (inventoryHas(_owl_eye_item)) then
      return
   end

   setOwlEyesPresent(false)
   inventoryAdd(_owl_eye_item)
   showDialogue("rubies_acquired")
end


------------------------------------------------------------------------------------------------------------------------
function update(dt)
   -- the mechanism lookup is not wired up yet while initialize() runs, so the shrine is set up on the first frame
   if (not _initialized) then
      _initialized = true
      initShrine()
   end
end


------------------------------------------------------------------------------------------------------------------------
function mechanismEvent(object_id, group_id, event_name, value)
   -- player pressed the action button in front of the shrine
   if (object_id == "shrine_rect" and event_name == "pressed" and value == "true") then
      takeOwlEyes()
   end

   -- the storm the owl was keeping asleep breaks loose once the player has read that he owns the rubies
   if (object_id == "rubies_acquired" and event_name == "dismissed") then
      setMechanismEnabled("thunderstorm", true, "weather")
   end
end


------------------------------------------------------------------------------------------------------------------------
function playerCollidesWithRect(rect_id)
end


------------------------------------------------------------------------------------------------------------------------
function playerCollidesWithSensorRect(rect_id)
end
