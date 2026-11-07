-- run from the repository root with lua tests/lua/test_progression_restore.lua
local enabled, visible, inventory, treasures, properties = {}, {}, {}, {}, {}
local awarded = 0
local camera_sequences = 0

function log() end
function setMechanismEnabled(id, value) enabled[id] = value end
function isMechanismEnabled(id) return enabled[id] == true end
function setMechanismVisible(id, value) visible[id] = value end
function inventoryHas(id) return inventory[id] == true end
function inventoryAdd(id) inventory[id] = true; awarded = awarded + 1 end
function inventoryRemove(id) inventory[id] = nil end
function hasTreasure(id) return treasures[id] == true end
function addTreasure(id) treasures[id] = true end
function getMechanismProperty(id, _, key)
   return properties[id] and properties[id][key]
end
function showDialogue() end
function writeLuaNodeProperty() end
function strikeThunderMechanism() end
function getMechanismRect() return {x = 0, y = 0, width = 24, height = 24} end
function clampCameraToRoom(x, y) return {x = x, y = y} end
function getRestingCameraCenter() return {x = 0, y = 0} end

local cutscene = {
   load = function() end,
   notify = function() camera_sequences = camera_sequences + 1 end
}
package.preload["data/level-catacombs/level_constants"] = function() return {} end
package.preload["data/scripts/cutscene"] = function() return cutscene end
package.preload["data/scripts/music_zones"] = function() return {} end
package.preload["data/level-catacombs/lever_camera_paths"] = function() return {} end
package.preload["data/level-catacombs/rift_puzzle"] = function() return {} end

local function reload_catacombs()
   dofile("data/level-catacombs/level.lua")
end

reload_catacombs()
initLocker()
assert(not visible.locker_open and not enabled.handle, "fresh locker must remain locked")
openLocker()
reload_catacombs()
initLocker()
assert(visible.locker_open and enabled.handle, "opened locker must retain its uncollected handle")

-- older saves can identify a spent key and an inserted handle without the new marker
treasures = {}
properties.locker_key = {active = false}
reload_catacombs()
initLocker()
assert(visible.locker_open and enabled.handle)
properties.lever_cell = {handle_available = true}
properties.handle = {active = false}
reload_catacombs()
initLocker()
assert(not enabled.lever_cell_dialogue and not enabled.locker_help)

openDrawer()
assert(inventory.solar_seal)
inventoryRemove("solar_seal")
reload_catacombs()
initDrawer()
local before = awarded
openDrawer()
assert(visible.drawer_open and not enabled.drawer_rect and awarded == before,
   "consumed seal must not be awarded again")
treasures = {}
properties.locked_box = {open = true}
reload_catacombs()
initDrawer()
openDrawer()
assert(awarded == before, "legacy open chest must identify an already searched drawer")

dofile("data/level-graveyard/level.lua")
takeOwlEyes()
assert(inventory.gems and enabled.thunderstorm)
inventoryRemove("gems")
treasures.catacombs_owl_eyes_inserted = true
dofile("data/level-graveyard/level.lua")
initShrine()
before = awarded
takeOwlEyes()
assert(not visible["owl-eyes"] and enabled.thunderstorm and not enabled.sinkhole_ground)
assert(awarded == before, "inserted rubies must not respawn")
treasures = {catacombs_owl_eyes_inserted = true}
dofile("data/level-graveyard/level.lua")
initShrine()
assert(not visible["owl-eyes"] and enabled.thunderstorm, "legacy inserted rubies must preserve the storm")

-- lever positions are engine state, while the first-use camera sequences are progress markers
treasures = {}
enabled.lever_spike_cannon = false
enabled.lever_spike_01 = true
local function reload_levers()
   local paths = dofile("data/level-catacombs/lever_camera_paths.lua")
   paths.init()
   return paths
end
local paths = reload_levers()
assert(enabled["ct-on-off-block-01"] and enabled["ct-crossroads-block10"])
enabled.lever_spike_cannon = true
paths.mechanismEvent("lever_spike_cannon", "state")
enabled.lever_spike_01 = false
paths.mechanismEvent("lever_spike_01", "state")
paths.update(20)
assert(not enabled["ct-on-off-block-01"] and not enabled["ct-crossroads-block10"])
assert(camera_sequences == 2)
paths = reload_levers()
assert(not enabled["ct-on-off-block-01"] and not enabled["ct-crossroads-block10"])
enabled.lever_spike_cannon = false
paths.mechanismEvent("lever_spike_cannon", "state")
enabled.lever_spike_01 = true
paths.mechanismEvent("lever_spike_01", "state")
assert(enabled["ct-on-off-block-01"] and enabled["ct-crossroads-block10"])
paths = reload_levers()
paths.mechanismEvent("lever_spike_cannon", "state")
paths.mechanismEvent("lever_spike_01", "state")
assert(camera_sequences == 2, "returning a lever to its default must not forget the first-use sequence")

treasures = {}
enabled.lever_spike_cannon = true
enabled.lever_spike_01 = false
reload_levers()
assert(treasures.catacombs_lever_spike_cannon_seen and treasures.catacombs_lever_spike_01_seen,
   "legacy lever positions must migrate the first-use flags")
print("PASS: locker, consumed seal, graveyard rubies, lever blocks and camera sequences, legacy saves")
