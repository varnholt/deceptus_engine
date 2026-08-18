-- shared level music handling
-- keeps the level music in sync with the area the player is in: entering a zone's sensor rect
-- crossfades to that zone's track, and once the player has reached the checkpoint a zone belongs
-- to, that zone's track becomes the track the level starts with. require this from any level
-- script that changes music while the level is running.
--
-- the level script has to hand over three things:
--
--    local music_zones = require "data/scripts/music_zones"
--
--    function initialize()
--       music_zones.configure({
--          default_track = "data/music/some_track.ogg",
--          zones = {
--             {sensor_rect = "zone_rect", track = "data/music/other_track.ogg", from_checkpoint = 1},
--          }
--       })
--    end
--
--    function update(dt)
--       if (not _initialized) then
--          music_zones.registerSensorRects()
--       end
--    end
--
--    function playerCollidesWithSensorRect(rect_id)
--       music_zones.playerCollidesWithSensorRect(rect_id)
--    end
--
-- configure() must be called from the level script's initialize function: the engine starts the
-- level music once loading has finished, which is after initialize but before the first update,
-- so that is the only place where the starting track can still be selected without the default
-- track being audible first. registerSensorRects() on the other hand has to wait for the first
-- update tick, because the engine's mechanism lookup is not wired up yet while initialize runs.

local MusicZones = {}

-- numeric constants passed to the engine's playMusic binding
local _transition_crossfade = 1
local _post_action_loop = 1

local _crossfade_duration_ms = 1000

local _default_track = nil
local _zones = {}         -- list of {sensor_rect, track, from_checkpoint}
local _current_track = nil

-- the track the level is supposed to start with: the track of the furthest zone the player has
-- already checkpointed into, or the default track when no zone applies yet.
local function trackForCheckpoint(checkpoint)
   local track = _default_track
   local best_checkpoint = nil

   for _, zone in ipairs(_zones) do
      if (zone.from_checkpoint and zone.from_checkpoint <= checkpoint) then
         if (not best_checkpoint or zone.from_checkpoint >= best_checkpoint) then
            best_checkpoint = zone.from_checkpoint
            track = zone.track
         end
      end
   end

   return track
end

-- called by the level script from its initialize function.
-- picks the track matching the reached checkpoint and hands it to the engine, which starts it
-- when level loading has finished.
function MusicZones.configure(config)
   _default_track = config.default_track
   _zones = config.zones or {}
   _crossfade_duration_ms = config.crossfade_duration_ms or _crossfade_duration_ms

   _current_track = trackForCheckpoint(getCheckpoint())

   if (_current_track) then
      setLevelMusic(_current_track)
   end
end

-- called by the level script from its first update tick.
-- subscribes to every configured zone's sensor rect.
function MusicZones.registerSensorRects()
   for _, zone in ipairs(_zones) do
      addSensorRectCallback(zone.sensor_rect)
   end
end

-- called by the level script when the player touches a sensor rect.
-- crossfades to the zone's track unless it is already the one playing.
-- returns true when the rect belonged to a music zone.
function MusicZones.playerCollidesWithSensorRect(rect_id)
   for _, zone in ipairs(_zones) do
      if (zone.sensor_rect == rect_id) then
         if (zone.track ~= _current_track) then
            _current_track = zone.track
            playMusic(zone.track, _transition_crossfade, _crossfade_duration_ms, _post_action_loop)
         end
         return true
      end
   end

   return false
end

return MusicZones
