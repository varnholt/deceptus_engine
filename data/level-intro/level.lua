local cutscene = require "data/scripts/cutscene"

function initialize()
   local actions = loadCutscene("data/level-intro/intro.json")
   cutscene.load(actions)
end

function update(dt)
   cutscene.update(dt)
end

function mechanismEvent(object_id, group, event_name, value)
   cutscene.notify(object_id .. "/" .. event_name)
end

function onEvent(event_name)
   cutscene.notify(event_name)
end
