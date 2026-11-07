-- run from the repository root with lua tests/lua/test_monk_encounter.lua
local treasures = {}
local shown = 0
local spoken = 0
local zoom = 1.0

package.preload["data/level-catacombs/level_constants"] = function() return {} end
package.preload["data/scripts/cutscene"] = function() return {} end
package.preload["data/scripts/music_zones"] = function()
   return {playerCollidesWithSensorRect = function() return false end}
end
package.preload["data/level-catacombs/lever_camera_paths"] = function()
   return {mechanismEvent = function() end}
end
package.preload["data/level-catacombs/rift_puzzle"] = function()
   return {
      playerCollidesWithSensorRect = function() return false end,
      mechanismEvent = function() end
   }
end

function hasTreasure(id) return treasures[id] == true end
function addTreasure(id) treasures[id] = true end
function log() end
function writeLuaNodeProperty() end
function setLuaNodeVisible(_, visible)
   if visible then shown = shown + 1 end
end
function setZoomFactor(value) zoom = value end
function lockPlayerControls() end
function lockPlayerOrientation() end
function playEventRecording() end
function showDialogue(id)
   assert(id == "monk")
   spoken = spoken + 1
end

local function reload_level()
   dofile("data/level-catacombs/level.lua")
end

reload_level()
playerCollidesWithSensorRect("monk_rect")
updateMonk(1.0)
assert(shown == 1)
assert(not hasTreasure(_monk_encounter_treasure), "unfinished encounter must remain replayable")

-- loading a checkpoint from before the encounter lets it play again
reload_level()
playerCollidesWithSensorRect("monk_rect")
updateMonk(1.0)
updateMonk(2.0)
updateMonk(2.0)
assert(shown == 2 and spoken == 1)
mechanismEvent("monk", "dialogues", "state", "hide")
updateMonk(3.0)
assert(zoom == 1.0)
assert(hasTreasure(_monk_encounter_treasure), "completion must enter saved player progress")

-- the checkpoint stores the collected progress markers with the player
local checkpoint_treasures = {}
for id, value in pairs(treasures) do checkpoint_treasures[id] = value end

playerCollidesWithSensorRect("monk_rect")
updateMonk(10.0)
assert(shown == 2 and spoken == 1, "re-entering must not replay the encounter")

treasures = checkpoint_treasures
reload_level()
playerCollidesWithSensorRect("monk_rect")
updateMonk(10.0)
updateMonk(10.0)
assert(shown == 2 and spoken == 1, "restored completion must prevent replay")
assert(not _player_intersected_with_monk_rect)
print("PASS: unfinished encounter, completion, repeat entry, restored progress")
