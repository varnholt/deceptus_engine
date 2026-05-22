# Cutscene System

Cutscenes in Deceptus are driven entirely from Lua, using a JSON file as the data source and a shared Lua helper (`data/scripts/cutscene.lua`) as the runtime. C++ provides only primitive operations; all sequencing and event handling lives in script.

---

## Architecture

```
intro.json          data file — list of timed and event-triggered actions
cutscene.lua        shared runner — loads, updates, and dispatches actions
level.lua           level script — wires engine callbacks into the runner
LevelScript (C++)   exposes primitives to Lua via lua_register
```

The runner is **event-based**: there is no blocking or yielding. Actions fire at a given time (`"at"`) or in response to a named event (`"on"`). When the engine notifies Lua of something (a dialogue being dismissed, a sprite arriving at its target, a fade completing), Lua forwards the event name to `cutscene.notify(...)` and the runner dispatches the next batch of queued actions.

---

## Setting Up a Cutscene Level

Every cutscene level needs a `level.lua` that requires the helper and wires the three engine callbacks:

```lua
local cutscene = require "data/scripts/cutscene"

function initialize()
   lockPlayerControls(999999)
   local actions = loadCutscene("data/level-intro/intro.json")
   cutscene.load(actions)
end

function update(dt)
   cutscene.update(dt)
end

function onMechanismEvent(object_id, group, event_name, value)
   cutscene.notify(object_id .. "/" .. event_name)
end

function onEvent(event_name)
   cutscene.notify(event_name)
end
```

`onMechanismEvent` receives events from engine mechanisms (dialogues, fades).
`onEvent` receives events raised directly by the engine, e.g. sprite arrival notifications.

---

## JSON Action Format

Each entry in the JSON array is either **timed** (fires at a time offset from the start) or **event-triggered** (fires when a named event arrives).

### Timed actions

```json
{ "at": 2.5, "action": "...", ... }
```

`"at"` is in seconds from the start of the cutscene. Multiple actions at the same time all fire on the same frame.

### Event-triggered actions

```json
{ "on": "some_event", "action": "...", ... }
{ "on": "some_event", "delay": 1.5, "action": "...", ... }
```

`"on"` is an event name (see [Event Names](#event-names) below). An optional `"delay"` (seconds) defers the action after the event arrives.

**Consecutive entries that share the same `"on"` value form one group.** The entire group fires together on the first occurrence of that event name. If the same event fires again later, the next group with that event name is consumed — this allows the same event (e.g. `"fade/out_done"`) to trigger different actions at different points in the sequence.

---

## Available Actions

### Camera and view

| Action | Parameters | Description |
|---|---|---|
| `set_camera_position` | `x`, `y` | Snaps the camera to a world pixel position and locks tracking there. |
| `unlock_camera` | — | Releases the camera lock and resumes normal player-tracking behaviour. |
| `fade_in` | `speed` | Fades from black to visible. Speed is alpha/sec (1.0 = one second). |
| `fade_out` | `speed` | Fades to black and holds. Call `fade_in` later to continue. |
| `set_hud_visible` | `visible` | Shows or hides the entire HUD overlay. |

### Player

| Action | Parameters | Description |
|---|---|---|
| `set_player_visible` | `visible` | Shows or hides the player sprite. |

### Sprites

Cutscene sprites are independent animated characters managed by the engine. They are separate from the player and from LuaNode objects.

| Action | Parameters | Description |
|---|---|---|
| `create_sprite` | `name`, `animation_file`, `animation`, `x`, `y`, `looped` | Creates a sprite and starts playing the given animation. `animation_file` is the path to the animation JSON (e.g. `data/sprites/animations.json`). `name` is used in all subsequent operations. Set `"looped": true` for continuously cycling animations. |
| `destroy_sprite` | `name` | Removes a sprite. |
| `set_sprite_animation` | `name`, `animation` | Switches to a different animation on an existing sprite. |
| `set_sprite_visible` | `name`, `visible` | Shows or hides a sprite without destroying it. |
| `move_sprite` | `name`, `x`, `y`, `speed`, `event` | Moves a sprite toward `(x, y)` at `speed` pixels/sec. When it arrives, raises the event named `event` (received via `onEvent` in the level script). |

Animation IDs follow the naming convention in `data/sprites/animations.json` and use `_l`/`_r` suffixes for left/right direction, e.g. `player_idle_r`, `player_run_l`.

### Dialogue

| Action | Parameters | Description |
|---|---|---|
| `show_dialogue` | `id` | Activates the first dialogue mechanism whose object id matches `id` (used as a regex pattern) and advances it to the next page. |

When a dialogue sequence is fully dismissed, the engine emits a `dismissed` event with the dialogue's object id. In the JSON this is written as `"on": "<object_id>/dismissed"`.

### Audio

| Action | Parameters | Description |
|---|---|---|
| `play_music` | `file`, `transition`, `duration_ms`, `post_action` | Queues a music track. `transition`: `"none"`, `"crossfade"`, `"fade_in_only"`. `post_action`: `"loop"`, `"stop"`. |
| `play_sound` | `file` | Plays a one-shot sound effect. |

### Level flow

| Action | Parameters | Description |
|---|---|---|
| `next_level` | — | Triggers the next-level transition. |

---

## Event Names

| Event | Source | Description |
|---|---|---|
| `fade/out_done` | Engine | Screen has faded to full black. Safe to reposition camera and swap sprites. |
| `fade/in_done` | Engine | Fade-in has completed. |
| `<object_id>/dismissed` | Dialogue | The named dialogue's last page was dismissed. |
| `<custom_event>` | `move_sprite` | Sprite arrived at its target. The name is whatever was passed in the `"event"` field of `move_sprite`. |

---

## Example Sequence

A minimal scene: show a character, walk it across the screen, then load the next level.

```json
[
   { "at": 0.0, "action": "set_hud_visible",     "visible": false },
   { "at": 0.0, "action": "set_player_visible",  "visible": false },
   { "at": 0.0, "action": "set_camera_position", "x": 500, "y": 300 },
   { "at": 0.0, "action": "create_sprite",        "name": "hero", "animation": "player_idle_r", "animation_file": "data/sprites/animations.json", "x": 400, "y": 300 },
   { "at": 0.0, "action": "fade_in",              "speed": 1.0 },

   { "at": 2.0, "action": "set_sprite_animation", "name": "hero", "animation": "player_run_r" },
   { "at": 2.0, "action": "move_sprite",           "name": "hero", "x": 600, "y": 300, "speed": 80, "event": "hero_arrived" },

   { "on": "hero_arrived", "action": "set_sprite_animation", "name": "hero", "animation": "player_idle_r" },
   { "on": "hero_arrived", "delay": 1.0, "action": "fade_out", "speed": 1.0 },

   { "on": "fade/out_done", "action": "next_level" }
]
```

---

## cutscene.lua API

| Function | Description |
|---|---|
| `cutscene.load(actions)` | Loads a table of action entries (as returned by `loadCutscene`). |
| `cutscene.update(dt)` | Advances the timer and fires any timed actions due this frame. Call from `update(dt)` in the level script. |
| `cutscene.notify(event_name)` | Delivers an event to the runner and dispatches the next matching group. Call from `onMechanismEvent` and `onEvent`. |
| `cutscene.stop()` | Stops the runner. |
