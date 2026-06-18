# Deceptus Level Scripts

Each level can have a `level.lua` script to drive logic that is not covered by standard mechanism behavior.
For example, you might want to close a door or activate an enemy when the player moves to a certain position.
All that 'custom' logic goes into the level script.


# The Level Script API

## `addCollisionRect`

Adds a collision rectangle that triggers when the player intersects.

|Parameter Position|Type|Description|
|-|-|-|
|1|int|x position relative to where the object has been placed|
|2|int|y position relative to where the object has been placed|
|3|int|Collision rect width|
|4|int|Collision rect height|
|return|int|Identifier of the collision rectangle|


## `addSensorRectCallback`

Adds a callback when the player intersects with a specified sensor rectangle.

|Parameter Position|Type|Description|
|-|-|-|
|1|string|Identifier of the sensor rect|


## `isMechanismEnabled`

Checks if a given mechanism is enabled.

|Parameter Position|Type|Description|
|-|-|-|
|1|string|Mechanism search pattern (regular expression)|
|2|string|Mechanism group (optional)|


## `setMechanismEnabled`

Sets a mechanism to be enabled or disabled based on the search pattern.

|Parameter Position|Type|Description|
|-|-|-|
|1|string|Search pattern (regular expression)|
|2|bool|Enabled flag|


## `toggle`

Toggles the enabled state of a mechanism.

|Parameter Position|Type|Description|
|-|-|-|
|1|string|Mechanism search pattern (regular expression)|
|2|string|Group name (optional)|


## `writeLuaNodeProperty`

Writes a property of another Lua node based on the provided parameters.

|Parameter Position|Type|Description|
|-|-|-|
|1|string|Mechanism search pattern (regular expression)|
|2|string|property key|
|3|string|property value|


## `setLuaNodeVisible`

Sets the visibility of a specified mechanism.

|Parameter Position|Type|Description|
|-|-|-|
|1|string|Mechanism search pattern (regular expression)|
|2|bool|Visible flag|


## `setLuaNodeActive`

Enables or disables the body of a Lua node.

|Parameter Position|Type|Description|
|-|-|-|
|1|string|Mechanism search pattern (regular expression)|
|2|bool|Active flag|


## `addPlayerSkill`

Adds a skill to the player (as specified in `constants.lua`).

|Parameter Position|Type|Description|
|-|-|-|
|1|int|Skill to add|


## `removePlayerSkill`

Removes a skill from the player (as specified in `constants.lua`).

|Parameter Position|Type|Description|
|-|-|-|
|1|int|Skill to remove|


## `addAchievement`

Marks an achievement as earned for the current player.

|Parameter Position|Type|Description|
|-|-|-|
|1|string|Achievement identifier (as defined in `data/config/achievements.json`)|


## `hasAchievement`

Returns whether the player has already earned a given achievement.

|Parameter Position|Type|Description|
|-|-|-|
|1|string|Achievement identifier (as defined in `data/config/achievements.json`)|
|return|bool|`true` if the achievement has been earned|


## `addTreasure`

Marks a treasure as collected for the current player.

|Parameter Position|Type|Description|
|-|-|-|
|1|string|Treasure identifier (as defined in `data/config/treasures.json`)|


## `hasTreasure`

Returns whether the player has already collected a given treasure.

|Parameter Position|Type|Description|
|-|-|-|
|1|string|Treasure identifier (as defined in `data/config/treasures.json`)|
|return|bool|`true` if the treasure has been collected|


## `inventoryAdd`

Adds an item to the player's inventory.

|Parameter Position|Type|Description|
|-|-|-|
|1|string|Inventory item identifier|


## `inventoryRemove`

Removes an item from the player's inventory.

|Parameter Position|Type|Description|
|-|-|-|
|1|string|Inventory item identifier|


## `inventoryHas`

Returns whether the player currently carries a given inventory item.

|Parameter Position|Type|Description|
|-|-|-|
|1|string|Inventory item identifier|
|return|bool|`true` if the item is present|


## `addPlayerHealth`

Increases the player's current health.

|Parameter Position|Type|Description|
|-|-|-|
|1|int|Amount of health to add|


## `addPlayerHealthMax`

Increases the player's maximum health.

|Parameter Position|Type|Description|
|-|-|-|
|1|int|Amount to add to the maximum|


## `giveWeaponBow`

Equips a bow as the player's active weapon. Takes no parameters.


## `giveWeaponGun`

Equips a gun as the player's active weapon. Takes no parameters.


## `giveWeaponSword`

Equips a sword as the player's active weapon. Takes no parameters.


## `isPlayerIntersectingSensorRect`

Returns whether the player is currently inside a named sensor rectangle.

|Parameter Position|Type|Description|
|-|-|-|
|1|string|Sensor rect object identifier|
|return|bool|`true` if the player intersects the sensor rect|


## `isMechanismVisible`

Returns whether a mechanism is currently visible.

|Parameter Position|Type|Description|
|-|-|-|
|1|string|Mechanism search pattern (regular expression)|
|2|string|Mechanism group (optional)|
|return|bool|`true` if the first matching mechanism is visible|


## `setMechanismVisible`

Shows or hides all mechanisms matching the search pattern.

|Parameter Position|Type|Description|
|-|-|-|
|1|string|Mechanism search pattern (regular expression)|
|2|bool|Visible flag|
|3|string|Mechanism group (optional)|


## `flashMechanism`

Triggers a colour flash on all matching `RingShaderLayer` mechanisms.

|Parameter Position|Type|Description|
|-|-|-|
|1|string|Mechanism search pattern (regular expression)|
|2|float|Red component (0.0–1.0)|
|3|float|Green component (0.0–1.0)|
|4|float|Blue component (0.0–1.0)|
|5|float|Fade-out duration in seconds|


## `getMechanismRect`

Returns the bounding rectangle of the first mechanism matching the search pattern, or `nil` if none is found.

|Parameter Position|Type|Description|
|-|-|-|
|1|string|Mechanism search pattern (regular expression)|
|2|string|Mechanism group (optional)|
|return|table or nil|Table with fields `x`, `y`, `width`, `height` in world pixels|


## `getCameraCenter`

Returns the current camera center in world pixel coordinates.

|Parameter Position|Type|Description|
|-|-|-|
|return|table|Table with fields `x` and `y`|


## `setCameraPosition`

Snaps the camera to a fixed world position and stops it from following the player.

|Parameter Position|Type|Description|
|-|-|-|
|1|float|Target x in world pixels|
|2|float|Target y in world pixels|


## `unlockCamera`

Releases a camera position lock set by `setCameraPosition` and resumes player tracking. Takes no parameters.


## `lockPlayerControls`

Disables all player input for a given duration.

|Parameter Position|Type|Description|
|-|-|-|
|1|int|Lock duration in milliseconds|


## `setCutsceneActive`

Sets or clears the `CutsceneActive` display flag. While active, spatial dialogue triggers are suppressed and camera panorama behaviour is blocked.

|Parameter Position|Type|Description|
|-|-|-|
|1|bool|`true` to activate cutscene mode, `false` to deactivate|


## `setPlayerVisible`

Shows or hides the player sprite.

|Parameter Position|Type|Description|
|-|-|-|
|1|bool|`true` to show, `false` to hide|


## `setInfoLayerVisible`

Shows or hides the HUD layer.

|Parameter Position|Type|Description|
|-|-|-|
|1|bool|`true` to show, `false` to hide|


## `fadeOut`

Fades the screen to black. The screen stays black until `fadeIn` is called.

|Parameter Position|Type|Description|
|-|-|-|
|1|float|Fade speed — alpha units per second (1.0 = one second for a full fade)|


## `fadeIn`

Fades the screen back in from black.

|Parameter Position|Type|Description|
|-|-|-|
|1|float|Fade speed — alpha units per second|


## `setAmbient`

Sets the ambient light colour for the level.

|Parameter Position|Type|Description|
|-|-|-|
|1|int|Red component (0–255)|
|2|int|Green component (0–255)|
|3|int|Blue component (0–255)|
|4|int|Alpha component (0–255)|


## `setZoomFactor`

Sets the camera zoom multiplier.

|Parameter Position|Type|Description|
|-|-|-|
|1|float|Zoom factor (1.0 = normal)|


## `playMusic`

Starts music playback with a specified transition and post-playback behaviour.

|Parameter Position|Type|Description|
|-|-|-|
|1|string|Music filename or identifier|
|2|int|Transition type — see values below|
|3|int|Transition duration in milliseconds|
|4|int|Post-playback action — see values below|

**Transition type values**

|Value|Constant|Description|
|-|-|-|
|0|`LetCurrentFinish`|Wait for the current track to end before starting the new one|
|1|`Crossfade`|Crossfade between the current and new track over the transition duration|
|2|`ImmediateSwitch`|Switch tracks instantly|
|3|`FadeOutThenNew`|Fade out the current track, then start the new one|

**Post-playback action values**

|Value|Constant|Description|
|-|-|-|
|0|`None`|Stop when the track ends|
|1|`Loop`|Restart the track when it ends|
|2|`PlayNext`|Play the next track in the list|


## `playSound`

Plays a one-shot sound effect.

|Parameter Position|Type|Description|
|-|-|-|
|1|string|Audio sample identifier|


## `playEventRecording`

Loads and replays a recorded player event stream.

|Parameter Position|Type|Description|
|-|-|-|
|1|string|Recording filename (`.dat` extension optional)|


## `nextLevel`

Triggers the transition to the next level. Takes no parameters.


## `createSprite`

Creates a managed cutscene sprite and starts playing an animation on it.

|Parameter Position|Type|Description|
|-|-|-|
|1|string|Unique sprite name used in subsequent sprite calls|
|2|string|Path to the animation settings JSON file|
|3|string|Animation identifier as defined in the settings file|
|4|float|Initial x position in world pixels|
|5|float|Initial y position in world pixels|
|6|bool|`true` to loop the animation continuously|


## `destroySprite`

Removes a managed cutscene sprite.

|Parameter Position|Type|Description|
|-|-|-|
|1|string|Sprite name passed to `createSprite`|


## `setSpriteAnimation`

Switches the active animation on a managed cutscene sprite.

|Parameter Position|Type|Description|
|-|-|-|
|1|string|Sprite name|
|2|string|Animation identifier|
|3|bool|`true` to loop the animation|


## `setSpriteVisible`

Shows or hides a managed cutscene sprite.

|Parameter Position|Type|Description|
|-|-|-|
|1|string|Sprite name|
|2|bool|`true` to show, `false` to hide|


## `moveSpriteAtSpeed`

Moves a managed cutscene sprite toward a target position at a constant speed. Fires an event when it arrives.

|Parameter Position|Type|Description|
|-|-|-|
|1|string|Sprite name|
|2|float|Target x in world pixels|
|3|float|Target y in world pixels|
|4|float|Speed in pixels per second|
|5|string|Event name forwarded to `onEvent` when the sprite reaches the target|


## `loadCutscene`

Loads a cutscene JSON file and returns its action table for use with the cutscene module (see `data/scripts/cutscene.md`).

|Parameter Position|Type|Description|
|-|-|-|
|1|string|Path to the cutscene JSON file|
|return|table|Array of action tables, one entry per cutscene step|


## `log`

Writes a message to the engine log (stdout). Useful for debugging scripts.

|Parameter Position|Type|Description|
|-|-|-|
|1|string|Message to log|


## `tr`

Returns the localised string for the given translation key.

|Parameter Position|Type|Description|
|-|-|-|
|1|string|Translation key|
|return|string|Localised string|


## `showDialogue`

Shows a modal dialogue. Accepts either a TMX object name or one or more inline page tables.

**TMX lookup** — finds the `Dialogue` mechanism matching `name` in the `dialogues` layer and shows it. All content and colors come from the TMX object.

|Parameter Position|Type|Description|
|-|-|-|
|1|string|Dialogue object name (regular expression)|

```lua
showDialogue("npc_intro")
```

**Inline pages** — each argument is a table describing one page. Pages are shown in order.

|Parameter Position|Type|Description|
|-|-|-|
|1..n|table|One table per page (see fields below)|

|Table Field|Type|Default|Description|
|-|-|-|-|
|`message`|string|`""`|Text to display. Supports `<br>` for line breaks and `<player>` for the player name.|
|`text_color`|string|`"#e8dbf3ff"`|Text color as `#AARRGGBB` hex.|
|`bg_color`|string|`"#2f0c4bff"`|Background color as `#AARRGGBB` hex.|
|`animate`|bool|`true`|Reveal text character by character.|
|`animate_speed`|number|`30.0`|Characters revealed per second when `animate` is `true`.|
|`x_px`|number|—|Horizontal position in world pixels. Requires `y_px`.|
|`y_px`|number|—|Vertical position in world pixels. Requires `x_px`.|

```lua
-- single page
showDialogue({message = "The door is sealed."})

-- single page with custom colors
showDialogue({
   message    = "Warning: the bridge is unstable.",
   text_color = "#ff4444ff",
   bg_color   = "#1a0000ff",
})

-- multi-page sequence
showDialogue(
   {message = "You found the ancient key."},
   {message = "It opens the chamber to the north.", animate = false}
)

-- page at a fixed screen position
showDialogue({message = "Over here!", x_px = 640, y_px = 180})
```


