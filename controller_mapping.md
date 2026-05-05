# Configurable Input Mapping — Code Review

## Motivation

All keyboard and controller button bindings were hardcoded in `playercontrols.cpp` as direct comparisons against SFML key constants and SDL button enum values. There was no way to change them without recompiling. This change moves the bindings into `data/config/controls.json` and provides the plumbing to load, persist, and use them at runtime.

---

## Files changed

| File | Change type |
|---|---|
| `src/game/config/inputconfiguration.h` | New |
| `src/game/config/inputconfiguration.cpp` | New |
| `data/config/controls.json` | New |
| `CMakeLists.txt` | Two lines added |
| `src/game/player/playercontrols.h` | One private method added |
| `src/game/player/playercontrols.cpp` | Three methods rewritten, one added |

---

## New: `InputConfiguration` (`src/game/config/inputconfiguration.h`)

A plain struct that holds three maps and follows the same singleton + JSON persistence pattern as `GameConfiguration`.

```cpp
struct InputConfiguration
{
    std::map<sf::Keyboard::Key, KeyPressed> _key_to_action;
    std::map<KeyPressed, sf::Keyboard::Key> _action_to_key;
    std::map<KeyPressed, int32_t>           _action_to_controller_button;
    ...
};
```

**Why three maps?**

- `_key_to_action` — used in `keyboardKeyPressed` / `keyboardKeyReleased`. The incoming value is an `sf::Keyboard::Key` and we need to find out which `KeyPressed` flag it maps to. Direct O(log n) lookup.
- `_action_to_key` — used in `forceSync`. That function calls `sf::Keyboard::isKeyPressed(key)` for each bound action, so it needs the forward direction (action → key).
- `_action_to_controller_button` — used when querying controller buttons. Maps a `KeyPressed` action flag to the SDL gamepad button integer.

`_key_to_action` is derived from `_action_to_key` (the JSON file stores only the forward direction). Both are populated together in `setDefaults()` and `deserialize()` to keep them in sync.

**`sf::Keyboard::Key` as a map key**

`sf::Keyboard::Key` is a scoped enum (`enum class`) in SFML 3, which means it has no implicit integer conversion and cannot be used as an `std::unordered_map` key without a custom hasher. `std::map` works because scoped enums do support `operator<` via their underlying integral type. This is why `std::map` is used here rather than `std::unordered_map`.

**`KeyPressed` as a map key**

`KeyPressed` is a plain (non-class) enum, so it converts to `int` implicitly. Both `std::map` and `std::unordered_map` would work; `std::map` is used for consistency.

---

## New: `inputconfiguration.cpp`

### Lookup tables (anonymous namespace)

Four static local maps live in an anonymous namespace:

| Function | Purpose |
|---|---|
| `getActionNameToFlagMap()` | `"jump"` → `KeyPressedJump`, etc. Used during JSON read. |
| `getActionFlagToNameMap()` | `KeyPressedJump` → `"jump"`, etc. Used during JSON write. Derived from the above via immediately-invoked lambda. |
| `getKeyboardNameToKeyMap()` | `"Space"` → `sf::Keyboard::Key::Space`, etc. Used during JSON read. Covers all letters (A–Z), digits (Num0–Num9), arrows, modifiers, F1–F12, numpad, and common special keys. |
| `getKeyToKeyboardNameMap()` | Reverse of the above. Used during JSON write. Also derived via immediately-invoked lambda. |
| `getControllerButtonNameToSdlMap()` | `"South"` → `SDL_GAMEPAD_BUTTON_SOUTH`, etc. Covers all 15 standard SDL gamepad buttons. |
| `getSdlToControllerButtonNameMap()` | Reverse of the above. Used during JSON write. |

The derived (reverse) maps are initialized once via `static const auto name = []() { ... }()` — a C++11 immediately-invoked lambda assigned to a `static const`. This guarantees thread-safe one-time initialization while building the reverse map from its canonical source.

### `setDefaults()`

Populates both keyboard maps and the controller button map with the original hardcoded values:

```
up       → Up arrow     / DpadUp
down     → Down arrow   / DpadDown
left     → Left arrow   (analog stick only for controller)
right    → Right arrow  (analog stick only for controller)
jump     → Space        / South (A)
slot_1   → LControl     / West  (X)
slot_2   → LAlt         / North (Y)
look     → LShift       (right analog stick for controller, not a button)
action   → Enter        / East  (B)
```

Note: `left`, `right`, and `look` have no controller button entry. Left/right movement via controller is driven entirely by the left analog stick and D-pad hat values in the existing `isMovingLeft()` / `isMovingRight()` logic, which was not touched. Camera pan (look) is driven by the right analog stick in `isCpanControlActive()`, also not touched.

### `deserializeFromFile()`

Mirrors the `GameConfiguration` read loop (character-by-character `ifstream`). If the file does not exist, `setDefaults()` is called and the defaults are written to disk so the file is present and editable after first launch.

### `deserialize()`

Starts from `setDefaults()` so any action absent from the JSON file retains its default. Each keyboard and controller section is then iterated with `json::items()`. Unknown action names or key names produce a `Log::Warning` and are skipped; a parse exception falls back to defaults entirely.

### `getInstance()`

```cpp
InputConfiguration& InputConfiguration::getInstance()
{
    static InputConfiguration __instance;
    if (!__initialized)
    {
        __instance.setDefaults();
        __defaults.setDefaults();
        __instance.deserializeFromFile();
        __initialized = true;
    }
    return __instance;
}
```

Identical structure to `GameConfiguration::getInstance()`. Lazy-initialized on first call, which happens the first time a key event is received (well before any frame matters).

---

## New: `data/config/controls.json`

The file ships with defaults so it exists from the start and the player can open it in any text editor.

```json
{
    "Controls": {
        "keyboard": {
            "action": "Enter",
            "down":   "Down",
            "jump":   "Space",
            "left":   "Left",
            "look":   "LShift",
            "right":  "Right",
            "slot_1": "LControl",
            "slot_2": "LAlt",
            "up":     "Up"
        },
        "controller": {
            "action": "East",
            "down":   "DpadDown",
            "jump":   "South",
            "slot_1": "West",
            "slot_2": "North",
            "up":     "DpadUp"
        }
    }
}
```

Valid keyboard names: any letter `A`–`Z`, `Num0`–`Num9`, `Numpad0`–`Numpad9`, `F1`–`F12`, `Up`, `Down`, `Left`, `Right`, `Space`, `Enter`, `Escape`, `Backspace`, `Tab`, `LShift`, `RShift`, `LControl`, `RControl`, `LAlt`, `RAlt`, `LSystem`, `RSystem`, `Home`, `End`, `Insert`, `Delete`, `PageUp`, `PageDown`, `Pause`, `Add`, `Subtract`, `Multiply`, `Divide`, `Semicolon`, `Comma`, `Period`, `Slash`, `Backslash`, `Apostrophe`, `Grave`, `Hyphen`, `Equal`, `LBracket`, `RBracket`, `Menu`.

Valid controller button names: `South`, `East`, `West`, `North`, `Back`, `Guide`, `Start`, `LeftStick`, `RightStick`, `LeftShoulder`, `RightShoulder`, `DpadUp`, `DpadDown`, `DpadLeft`, `DpadRight`.

---

## Changes to `playercontrols.cpp`

### `keyboardKeyPressed` — before

```cpp
if      (key == sf::Keyboard::Key::Space)    { _keys_pressed |= KeyPressedJump;   }
else if (key == sf::Keyboard::Key::LShift)   { _keys_pressed |= KeyPressedLook;   }
else if (key == sf::Keyboard::Key::Up)       { _keys_pressed |= KeyPressedUp;     }
// ... six more hardcoded branches
```

### `keyboardKeyPressed` — after

```cpp
const auto& key_to_action = InputConfiguration::getInstance()._key_to_action;
const auto found_action = key_to_action.find(key);
if (found_action != key_to_action.end())
{
    _keys_pressed |= found_action->second;
}
```

`keyboardKeyReleased` follows the same pattern, clearing the bit instead of setting it. No other logic in those methods changed.

### `forceSync` — before

Nine individual `sf::Keyboard::isKeyPressed(hardcoded_key)` checks.

### `forceSync` — after

```cpp
const auto& action_to_key = InputConfiguration::getInstance()._action_to_key;
for (const auto& [action_flag, keyboard_key] : action_to_key)
{
    if (sf::Keyboard::isKeyPressed(keyboard_key))
    {
        _keys_pressed |= action_flag;
    }
}
```

### New private helper: `isControllerActionPressed`

```cpp
bool PlayerControls::isControllerActionPressed(KeyPressed action) const
{
    const auto& controller_map = InputConfiguration::getInstance()._action_to_controller_button;
    const auto found_button = controller_map.find(action);
    if (found_button != controller_map.end())
    {
        return isControllerButtonPressed(found_button->second);
    }
    return false;
}
```

This sits between the existing `isControllerButtonPressed(int32_t)` (which takes a raw SDL button index) and the six action-level query methods. Without it each of those six methods would repeat the same three-line map lookup.

### `isButtonAPressed`, `isButtonBPressed`, `isButtonXPressed`, `isButtonYPressed`, `isUpButtonPressed`, `isDownButtonPressed`

Each method previously had one hardcoded SDL constant:

```cpp
// before
return isControllerButtonPressed(SDL_GAMEPAD_BUTTON_SOUTH);

// after
return isControllerActionPressed(KeyPressedJump);
```

The structure of each method (state check → keyboard bitmask check → controller check) is unchanged.

---

## What was intentionally not changed

- **`menu.cpp`** — the menu maps controller buttons directly to `sf::Keyboard::Key::Up/Down/Enter/Escape`. These are menu navigation keys, not gameplay actions, and are a separate concern from the `KeyPressed` action system.
- **Analog stick logic** — `isMovingLeft()`, `isMovingRight()`, `isMovingUp()`, `isMovingDown()`, `isCpanControlActive()`, `isBendDownActive()`, `readControllerNormalizedHorizontal()` all read raw axis values and D-pad hat state. None of that is a simple button lookup, so it was left alone.
- **`lockAll` / `lockState`** — existing behavior unchanged.
- **Event recording** — `EventSerializer` records raw `sf::Event` objects, so it is agnostic to what the keys mean. Replayed events flow through `keyboardKeyPressed` / `keyboardKeyReleased` and pick up the current bindings automatically.
