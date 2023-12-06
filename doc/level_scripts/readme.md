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



