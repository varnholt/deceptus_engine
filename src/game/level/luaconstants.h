#pragma once

/// \brief lua callback name used when a LuaNode collides with the player.
#define FUNCTION_COLLISION_WITH_PLAYER "collisionWithPlayer"
/// \brief lua callback name used when a LuaNode receives damage.
#define FUNCTION_HIT "hit"
/// \brief lua callback name used during LuaNode initialization.
#define FUNCTION_INITIALIZE "initialize"
/// \brief lua callback name used for mechanism event forwarding in level scripts.
#define FUNCTION_MECHANISM_EVENT "mechanismEvent"
/// \brief lua callback name used for mechanism enabled-state updates in level scripts.
#define FUNCTION_MECHANISM_ENABLED "mechanismEnabled"
/// \brief lua callback name used when engine code updates a LuaNode position.
#define FUNCTION_MOVED_TO "movedTo"
/// \brief lua callback name used when the player enters a registered level-script collision rect.
#define FUNCTION_PLAYER_COLLIDES_WITH_RECT "playerCollidesWithRect"
/// \brief lua callback name used when the player enters a registered sensor rect.
#define FUNCTION_PLAYER_COLLIDES_WITH_SENSOR_RECT "playerCollidesWithSensorRect"
/// \brief lua callback name used when engine code updates the player position for scripts.
#define FUNCTION_PLAYER_MOVED_TO "playerMovedTo"
/// \brief lua callback name used when the player receives an extra pickup.
#define FUNCTION_PLAYER_RECEIVED_EXTRA "playerReceivedExtra"
/// \brief lua callback name used when the player receives an inventory item.
#define FUNCTION_PLAYER_RECEIVED_ITEM "playerReceivedItem"
/// \brief lua callback name used when scripts decide whether an inventory item can be used.
#define FUNCTION_PLAYER_USED_ITEM "playerUsedItem"
/// \brief lua callback name used to ask scripts to publish initial properties.
#define FUNCTION_RETRIEVE_PROPERTIES "retrieveProperties"
/// \brief lua callback name used to forward movement paths to scripts.
#define FUNCTION_SET_PATH "setPath"
/// \brief lua callback name used to provide initial spawn position to scripts.
#define FUNCTION_SET_START_POSITION "setStartPosition"
/// \brief lua callback name used when a script timer expires.
#define FUNCTION_TIMEOUT "timeout"
/// \brief lua callback name used every frame to update scripted behavior.
#define FUNCTION_UPDATE "update"
/// \brief lua callback name used to forward key-value properties to scripts.
#define FUNCTION_WRITE_PROPERTY "writeProperty"
/// \brief lua callback name used when a LuaNode is smashed.
#define FUNCTION_SMASHED "smashed"
