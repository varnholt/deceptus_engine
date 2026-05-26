# Cutscene System

The cutscene system is a Lua module (`data/scripts/cutscene.lua`) that runs a sequence of
time-based and event-based actions loaded from a JSON file.

## Level script setup

```lua
local cutscene = require "data/scripts/cutscene"

function initialize()
   local actions = loadCutscene("data/my-level/cutscene.json")
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
```

## Cutscene module API

| Function | Description |
|----------|-------------|
| `cutscene.load(actions)` | Loads the action table returned by `loadCutscene` and resets all timing state |
| `cutscene.update(dt)` | Advances the cutscene clock; call every frame from `update(dt)` |
| `cutscene.notify(event_name)` | Fires a named event, triggering any entries registered with `"on": "<event_name>"` |
| `cutscene.stop()` | Halts the cutscene; no further time-based or event-based entries are processed |

## Action scheduling

Each entry in the JSON array is either **time-based** or **event-based**.

| Field | Type | Description |
|-------|------|-------------|
| `at` | number | Fire at this elapsed time in seconds (time-based) |
| `on` | string | Fire when this event is notified (event-based) |
| `delay` | number | Optional extra delay in seconds before executing |
| `action` | string | The action to perform (see below) |

Time-based and event-based entries are mutually exclusive per entry.
Consecutive entries sharing the same `on` value form one group that fires together on a
single occurrence of the event and is then consumed.

## Events

Events are string names notified either by the engine or by actions that produce a
completion event. An event fires all entries registered with `"on": "<event_name>"`.

| Source | Event name |
|--------|-----------|
| `move_sprite` with `"event"` field | the value of the `event` field |
| `move_camera` with `"event"` field | the value of the `event` field |
| Dialogue dismissed by player | `<dialogue_id>/dismissed` |
| Fade out complete | `fade/out_done` |
| Fade in complete | `fade/in_done` |

## Actions

### Camera

#### `set_camera_position`
Instantly moves the camera to a position.

```json
{ "at": 0.0, "action": "set_camera_position", "x": 440, "y": 164 }
```

| Field | Type | Description |
|-------|------|-------------|
| `x` | number | Camera x position in pixels |
| `y` | number | Camera y position in pixels |

#### `move_camera`
Smoothly moves the camera to a position over time. Fires an optional event on arrival.
Only one camera move can be active at a time; a new one cancels any in-progress move.

```json
{ "on": "some_event", "action": "move_camera", "x": 600, "y": 200, "duration_s": 2.0, "easing": "ease_in_out", "event": "camera_done" }
```

| Field | Type | Description |
|-------|------|-------------|
| `x` | number | Target camera x position in pixels |
| `y` | number | Target camera y position in pixels |
| `duration_s` | number | Travel time in seconds |
| `easing` | string | `linear`, `ease_in`, `ease_out`, or `ease_in_out` |
| `event` | string | *(optional)* Event to fire when move completes |

#### `unlock_camera`
Releases the fixed camera position and returns control to the engine's camera system.

```json
{ "on": "some_event", "action": "unlock_camera" }
```

### Zoom

#### `set_zoom`
Sets the camera zoom factor immediately.

```json
{ "at": 1.0, "action": "set_zoom", "factor": 0.75 }
```

| Field | Type | Description |
|-------|------|-------------|
| `factor` | number | Zoom multiplier (1.0 = normal, < 1.0 = zoomed in) |

### Sprites

Sprites are transient animated objects created by the cutscene system, independent of
the level's tile layers and mechanisms.

#### `create_sprite`

```json
{ "at": 0.0, "action": "create_sprite", "name": "player_left", "animation_file": "data/sprites/animations.json", "animation": "player_run_r", "x": 40, "y": 188, "looped": true }
```

| Field | Type | Description |
|-------|------|-------------|
| `name` | string | Unique identifier for this sprite |
| `animation_file` | string | Path to the animations JSON |
| `animation` | string | Animation name within that file |
| `x` | number | Initial x position in pixels |
| `y` | number | Initial y position in pixels |
| `looped` | bool | Whether the animation loops |

#### `destroy_sprite`

```json
{ "on": "some_event", "action": "destroy_sprite", "name": "player_left" }
```

#### `set_sprite_animation`

```json
{ "on": "some_event", "action": "set_sprite_animation", "name": "player_left", "animation": "player_idle_r", "looped": true }
```

#### `set_sprite_visible`

```json
{ "on": "some_event", "action": "set_sprite_visible", "name": "player_left", "visible": false }
```

#### `move_sprite`
Moves a sprite toward a target position at a constant speed. Fires an optional event on arrival.

```json
{ "at": 1.5, "action": "move_sprite", "name": "player_left", "x": 410, "y": 188, "speed": 80, "event": "player_left_arrived" }
```

| Field | Type | Description |
|-------|------|-------------|
| `name` | string | Sprite to move |
| `x` | number | Target x in pixels |
| `y` | number | Target y in pixels |
| `speed` | number | Movement speed in pixels per second |
| `event` | string | *(optional)* Event to fire on arrival |

### Fading

#### `fade_out`

```json
{ "on": "some_event", "action": "fade_out", "speed": 1.0 }
```

Fires `fade/out_done` when complete.

#### `fade_in`

```json
{ "on": "some_event", "action": "fade_in", "speed": 1.0 }
```

Fires `fade/in_done` when complete.

| Field | Type | Description |
|-------|------|-------------|
| `speed` | number | Fade speed (higher = faster) |

### Dialogue

#### `show_dialogue`
Shows a dialogue box defined as an object in the level's `dialogues` objectgroup.
Fires `<id>/dismissed` when the player closes it.

```json
{ "on": "some_event", "action": "show_dialogue", "id": "intro_chat" }
```

### Audio

#### `play_music`

```json
{ "at": 0.0, "action": "play_music", "file": "data/music/track.ogg", "transition": "crossfade", "duration_ms": 1000, "post_action": "loop" }
```

| Field | Type | Description |
|-------|------|-------------|
| `file` | string | Path to the audio file |
| `transition` | string | `let_current_finish`, `crossfade`, `immediate`, or `fade_out_then_new` |
| `duration_ms` | number | Transition duration in milliseconds |
| `post_action` | string | `none`, `loop`, or `play_next` |

#### `play_sound`

```json
{ "on": "some_event", "action": "play_sound", "id": "coin_pickup" }
```

### Visibility

#### `set_player_visible`

```json
{ "at": 0.0, "action": "set_player_visible", "visible": false }
```

#### `set_info_layer_visible`
Shows or hides the HUD / info layer. State persists across level transitions.

```json
{ "at": 0.0, "action": "set_info_layer_visible", "visible": false }
```

### Level flow

#### `next_level`
Transitions to the next level defined in `data/config/levels.json`.

```json
{ "on": "fade/out_done", "action": "next_level" }
```

---

## Level script callbacks

These functions are called by the engine if defined in the level script. All are optional.

| Function | Arguments | Description |
|----------|-----------|-------------|
| `initialize()` | — | Called once when the level starts, before the first `update` |
| `update(dt)` | `dt: number` | Called every frame; `dt` is elapsed seconds |
| `mechanismEvent(object_id, group, event_name, value)` | strings + value | Fired when a mechanism emits an event (e.g. dialogue dismissed, sensor triggered) |
| `mechanismEnabled(object_id, group, enabled)` | string, string, bool | Fired when a mechanism's enabled state changes |
| `onEvent(event_name)` | `event_name: string` | Fired by the engine for raw events (e.g. sprite arrival) |
| `playerReceivedItem(item)` | `item: string` | Fired when the player gains an inventory item |
| `playerUsedItem(item)` | `item: string` → `bool` | Fired when the player uses an item; return `true` to consume it |
| `playerReceivedExtra(extra_name)` | `extra_name: string` | Fired when the player picks up an Extra mechanism |
| `playerCollidesWithRect(rect_id)` | `rect_id: number` | Fired each frame the player overlaps a rect registered with `addCollisionRect` |
| `playerCollidesWithSensorRect(rect_id)` | `rect_id: string` | Fired when the player enters a sensor rect registered with `addSensorRectCallback` |
| `writeProperty(key, value)` | string, string | Called at startup for each property set on the level object in Tiled |

---

## Lua functions

These are available anywhere in a level script.

### Player

```lua
setPlayerVisible(visible)          -- show or hide the player sprite
lockPlayerControls(duration_ms)    -- disable all input for the given number of milliseconds
addPlayerHealth(amount)            -- add health points to the player's current health
addPlayerHealthMax(amount)         -- increase the player's maximum health
addPlayerSkill(skill_bitmask)      -- set skill flags (OR into current skills)
removePlayerSkill(skill_bitmask)   -- clear skill flags (AND NOT into current skills)
giveWeaponBow()                    -- equip a bow and make it the active weapon
giveWeaponGun()                    -- equip a gun and make it the active weapon
giveWeaponSword()                  -- equip a sword and make it the active weapon
```

### State

```lua
addAchievement(identifier)         -- mark an achievement as earned
hasAchievement(identifier)         -- returns true if the achievement has been earned
addTreasure(identifier)            -- mark a treasure as collected
hasTreasure(identifier)            -- returns true if the treasure has been collected
inventoryAdd(item)                 -- add an item to the player's inventory
inventoryRemove(item)              -- remove an item from the player's inventory
inventoryHas(item)                 -- returns true if the item is in the inventory
```

### Mechanisms

The `search_pattern` argument is a regex matched against mechanism object IDs.
The optional `group` argument restricts the search to a specific layer name (e.g. `"fans"`).

```lua
setMechanismEnabled(pattern, enabled [, group])   -- enable or disable matching mechanisms
isMechanismEnabled(pattern [, group])             -- returns enabled state of first match
setMechanismVisible(pattern, visible [, group])   -- show or hide matching mechanisms
isMechanismVisible(pattern [, group])             -- returns visible state of first match
toggle(pattern [, group])                         -- toggle enabled state of matching mechanisms
flashMechanism(pattern, r, g, b, duration_s)      -- trigger a color flash on matching RingShaderLayer mechanisms
                                                  -- r/g/b are floats 0.0–1.0
```

### Lighting

```lua
setAmbient(r, g, b, a)   -- set the level's ambient light color; each channel is 0–255
```

### Lua nodes

The `search_pattern` is a regex matched against Lua node names.

```lua
writeLuaNodeProperty(pattern, key, value)   -- call writeProperty(key, value) on matching nodes
setLuaNodeVisible(pattern, visible)         -- show or hide matching nodes
setLuaNodeActive(pattern, active)           -- enable or disable physics on matching nodes
```

### Sensor rects and collision

```lua
-- Register a callback so playerCollidesWithSensorRect fires when the player enters the rect.
addSensorRectCallback(pattern)

-- Returns true if the player is currently inside the named sensor rect.
isPlayerIntersectingSensorRect(id)

-- Register an axis-aligned collision rect; returns an integer ID.
-- playerCollidesWithRect(id) fires each frame the player overlaps it.
addCollisionRect(x, y, width, height)
```

### Audio and camera

These are available directly and mirror the JSON actions of the same name.

```lua
playMusic(file, transition, duration_ms, post_action)
playSound(id)
setCameraPosition(x, y)
unlockCamera()
setZoomFactor(factor)
```

`transition` and `post_action` take integer values; use the constants in `cutscene.lua`'s
`_music_transition` / `_music_post_action` tables when calling from your own Lua, or pass
the JSON string form from the cutscene action.

### Utilities

```lua
playEventRecording(filename)   -- replay a recorded player input file (.dat)
tr(string)                     -- return the localized translation of string
log(message)                   -- write a debug message to the log
```

---

## Full example

See `data/level-intro/intro.json` for a complete working cutscene.
