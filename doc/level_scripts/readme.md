# callback list

```cpp
lua_register(_lua_state, "addCollisionRect", ::addCollisionRect);
lua_register(_lua_state, "addPlayerSkill", ::addPlayerSkill);
lua_register(_lua_state, "addSensorRectCallback", ::addSensorRectCallback);
lua_register(_lua_state, "giveWeaponBow", ::giveWeaponBow);
lua_register(_lua_state, "giveWeaponGun", ::giveWeaponGun);
lua_register(_lua_state, "giveWeaponSword", ::giveWeaponSword);
lua_register(_lua_state, "isMechanismEnabled", ::isMechanismEnabled);
lua_register(_lua_state, "removePlayerSkill", ::removePlayerSkill);
lua_register(_lua_state, "setLuaNodeActive", ::setLuaNodeActive);
lua_register(_lua_state, "setLuaNodeVisible", ::setLuaNodeVisible);
lua_register(_lua_state, "setMechanismEnabled", ::setMechanismEnabled);
lua_register(_lua_state, "toggle", ::toggle);
lua_register(_lua_state, "writeLuaNodeProperty", ::writeLuaNodeProperty);
```

## addCollisionRect

```cpp
/**
 * @brief addCollisionRect add a collision rect that fires when the player intersects
 * @param state lua state
 *    param 1: x position relative to where the object has been placed
 *    param 2: y position relative to where the object has been placed
 *    param 3: collision rect width
 *    param 4: collision rect height
 * @return collision rect id
 */
int32_t addCollisionRect(lua_State* state)
{
}
```

## addSensorRectCallback

```cpp
/**
 * @brief addSensorRectCallback add a callback when player intersects with a given sensor rect
 * @param state lua state
 *    param 1: identifier of the sensor rect
 * @return error code
 */
int32_t addSensorRectCallback(lua_State* state)
{
}
```

## isMechanismEnabled
```cpp
/**
 * @brief isMechanismEnabled check if a given mechanism is enabled
 * @param state lua state
 *    param 1: mechanism ID
 *    return \c true if mechanism is enabled
 * @return error code
 */
int32_t isMechanismEnabled(lua_State* state)
{
}
```

## setMechanismEnabled

```cpp
/**
 * @brief setMechanismEnabled set a mechanism node to enabled/disabled
 * @param state lua state
 *    param 1: search pattern
 *    param 2: enabled flag
 * @return error code
 */
int32_t setMechanismEnabled(lua_State* state)
{
}
```

## toggle

```cpp
/**
 * @brief toggle toggle a mechanism
 * @param state lua state
 *    param 1: mechanism name
 *    param 2: group name
 * @return error code
 */
int32_t toggle(lua_State* state)
{
}
```

## writeLuaNodeProperty

```cpp
/**
 * @brief writeLuaNodeProperty write a property of another lua node
 * @param state lua state
 *    param 1: property key
 *    param 2: property value
 *    param 3: mechanism name
 *    param 4: group name
 * @return error code
 */
int32_t writeLuaNodeProperty(lua_State* state)
{
}
```

## setLuaNodeVisible

```cpp
/**
 * @brief setLuaNodeVisible write a property of another lua node
 * @param state lua state
 *    param 1: mechanism name
 *    param 2: visible flag
 * @return error code
 */
int32_t setLuaNodeVisible(lua_State* state)
{
}
```

## setLuaNodeActive

```cpp
/**
 * @brief setLuaNodeActive write a property of another lua node
 * @param state lua state
 *    param 1: mechanism name
 *    param 2: active flag
 * @return error code
 */
int32_t setLuaNodeActive(lua_State* state)
{
}
```

## addPlayerSkill

```cpp
/**
 * @brief addPlayerSkill add a skill to the player
 * @param state lua state
 *    param 1: skill to add
 * @return error code
 */
int32_t addPlayerSkill(lua_State* state)
{
}
```

## removePlayerSkill

```cpp
/**
 * @brief removePlayerSkill add a skill to the player
 * @param state lua state
 *    param 1: skill to add
 * @return error code
 */
int32_t removePlayerSkill(lua_State* state)
{
}
```

