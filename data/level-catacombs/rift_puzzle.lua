-- the rift: a room that only lets the player through when its four exits are taken in the order the
-- inscriptions in the scriptorium give
--
--    "Before all things, there is time."            -> the hourglass, lower right
--    "In time, all are called to judgment."         -> the scales, lower left
--    "Through judgment, truth is unveiled."         -> the eye, upper left
--    "And in truth, all return to the end."         -> the skull, upper right
--
-- the exits are the strips just outside the room rect, behind its walls, where the camera no longer
-- follows. a correct exit puts the player back into the rift on the opposite side, the wrong one back
-- to the scriptorium where the order can be read up again, and the last one through to ct-room8.
-- require this from the catacombs level script:
--
--    local rift_puzzle = require "data/level-catacombs/rift_puzzle"
--
--    function initialize()
--       rift_puzzle.initialize()
--    end
--
--    function update(dt)
--       if (not _initialized) then
--          rift_puzzle.init()
--       end
--    end
--
--    function mechanismEvent(object_id, group_id, event_name, value)
--       rift_puzzle.mechanismEvent(object_id, event_name)
--    end
--
--    function playerCollidesWithSensorRect(rect_id)
--       if (rift_puzzle.playerCollidesWithSensorRect(rect_id)) then
--          return
--       end
--    end
--
-- init() registers the sensor rects, which needs the engine's mechanism lookup, so it belongs in the
-- level script's first update tick rather than in initialize().

local RiftPuzzle = {}

local _correct_sample = "rift_exit_correct.ogg"
local _wrong_sample = "rift_exit_wrong.ogg"
local _solved_sample = "rift_solved.ogg"

-- where the scriptorium's west gate leads
local _start_position_px = {x = 6616, y = 1624}

-- right inside ct-room8's lower left entrance
local _solved_position_px = {x = 7118, y = 1624}

-- right inside the scriptorium's west gate, where a wrong exit and leaving ct-room8 the way it was
-- entered lead back to
local _library_position_px = {x = 6048, y = 2128}

local _exit_order = {
   "rift_exit_time",
   "rift_exit_judgment",
   "rift_exit_truth",
   "rift_exit_end",
}

-- each exit lets a correct guess back in on the opposite side of the rift
local _exit_wrap_positions_px = {
   rift_exit_time = {x = 6061, y = 1624},
   rift_exit_judgment = {x = 6658, y = 1624},
   rift_exit_truth = {x = 6658, y = 1456},
   rift_exit_end = {x = 6061, y = 1456},
}

-- the scriptorium's west gate, and ct-room8's way back to the scriptorium
local _entrance_rect = "rift_entrance"
local _return_rect = "rift_return"

local _exits_passed = 0
local _transition_running = false
local _sample_on_arrival = nil


------------------------------------------------------------------------------------------------------------------------
local function transitionTo(position_px, sample)
   _transition_running = true
   _sample_on_arrival = sample
   transitionPlayerTo(position_px.x, position_px.y)
end


------------------------------------------------------------------------------------------------------------------------
local function passExit(rect_id)
   if (rect_id ~= _exit_order[_exits_passed + 1]) then
      log(string.format("rift: %s is the wrong exit after %d correct ones", rect_id, _exits_passed))
      _exits_passed = 0
      transitionTo(_library_position_px, _wrong_sample)
      return
   end

   _exits_passed = _exits_passed + 1

   if (_exits_passed == #_exit_order) then
      log("rift: solved")
      _exits_passed = 0
      transitionTo(_solved_position_px, _solved_sample)
      return
   end

   log(string.format("rift: %s is correct, %d of %d", rect_id, _exits_passed, #_exit_order))
   transitionTo(_exit_wrap_positions_px[rect_id], _correct_sample)
end


------------------------------------------------------------------------------------------------------------------------
function RiftPuzzle.initialize()
   -- samples have to be loaded before they can be played
   addSample(_correct_sample)
   addSample(_wrong_sample)
   addSample(_solved_sample)
end


------------------------------------------------------------------------------------------------------------------------
function RiftPuzzle.init()
   for _, rect_id in ipairs(_exit_order) do
      addSensorRectCallback(rect_id)
   end
   addSensorRectCallback(_entrance_rect)
   addSensorRectCallback(_return_rect)
end


------------------------------------------------------------------------------------------------------------------------
-- returns true when the sensor rect belongs to the rift
function RiftPuzzle.playerCollidesWithSensorRect(rect_id)
   local is_exit = (_exit_wrap_positions_px[rect_id] ~= nil)

   if (not is_exit and rect_id ~= _entrance_rect and rect_id ~= _return_rect) then
      return false
   end

   -- the player stands still in the strip while the screen fades, so this is the same exit again
   if (_transition_running) then
      return true
   end

   if (is_exit) then
      passExit(rect_id)
      return true
   end

   -- whoever comes in from the scriptorium starts over
   _exits_passed = 0

   if (rect_id == _entrance_rect) then
      transitionTo(_start_position_px, nil)
   else
      transitionTo(_library_position_px, nil)
   end

   return true
end


------------------------------------------------------------------------------------------------------------------------
function RiftPuzzle.mechanismEvent(object_id, event_name)
   if (object_id ~= "player_transition") then
      return
   end

   -- the verdict is heard while the screen is black, before the player sees where it led
   if (event_name == "moved" and _sample_on_arrival ~= nil) then
      playSound(_sample_on_arrival)
      _sample_on_arrival = nil
   elseif (event_name == "done") then
      _transition_running = false
   end
end


return RiftPuzzle
