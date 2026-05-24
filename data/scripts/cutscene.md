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

## Full example

See `data/level-intro/intro.json` for a complete working cutscene.
