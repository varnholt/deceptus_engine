# Deceptus Engine Lua Interface

The Deceptus Engine comes with an interface written in Lua that lets you define the visuals and behavior of enemies inside the game.
The decision to go for Lua has been made as non-C++ programmers should be able to create their own enemies. Also creating or changing the behavior of enemies should be possible without making any change in the game engine.


# Creating Your First Own Enemy

Implementing your own enemy isn't rocket science. You have a function (a chunk of code) called `update`. That function is invoked by the game engine every frame.
So every frame you can decide whether you want to move left, right, jump, shoot or whatever action that seems suitable. This is basically how all AIs work: Look around you, decide what's the best thing to do and act upon it.

Before reading any further - yes, you will have to have some basic understanding of the Lua programming language. There's heaps of information on the web. To get started, just check out lua.org: https://www.lua.org/pil/1.html

## Setting Up Your Enemy

### Set Up Your Script

The first thing to do is to create a new `lua` file, e.g. `tutorial.lua`, inside the folder `scripts/enemies`.
Reference this file inside inside your `level.json` and set up a reasonable start position.

```json
  "enemies": [
    {"script": "tutorial.lua", "startposition": [50, 50]}
  ]
```

Now edit `tutorial.lua` and add an empty implementation for the absolute minimum functionality a Lua script can have:

```lua
function initialize()
   print("tutorial.lua initialized")
end
```

```lua
function update(dt)
end
```

Now, when you start the game and load your level, you should see the output 'tutorial.lua initialized' in the game's debug output.
That means the script has been loaded correctly. The next thing to do is to start implementing the actual 'business logic'.

### Initializing Your Script

### Bringing Your Script to Life


# The Lua API

`addSample`
`addShapeCircle`
`addShapeRect`
`addSprite`
`addWeapon`
`boom`
`damage`
`damageRadius`
`debug`
`die`
`fireWeapon`
`getLinearVelocity`
`isPhsyicsPathClear`
`makeDynamic`
`makeStatic`
`playDetonationAnimation`
`playSample`
`queryAABB`
`queryRayCast`
`registerHitAnimation`
`setActive`
`setDamage`
`setGravityScale`
`setLinearVelocity`
`setSpriteOffset`
`setSpriteOrigin`
`setTransform`
`setZ`
`timer`
`updateKeysPressed`
`updateProjectileAnimation`
`updateProjectileTexture`
`updateProperties`
`updateSpriteRect`



```cpp

/**
 * @brief updateProperties
 * @param state lua state
 *    param 1 key
 *    param 2 value
 *    param n key
 *    param n + 1 value
 * @return error code
 */
int32_t updateProperties(lua_State* state)
{
}


/**
 * @brief updateSpriteRect update node's sprite rect
 * @param state lua state
 *    param 1: id of sprite
 *    param 2: x position of sprite
 *    param 3: y position of sprite
 *    param 4: sprite width
 *    param 5: sprite height
 * @return error code
 */
int32_t updateSpriteRect(lua_State* state)
{
}


/**
 * @brief queryAABB do an aabb query
 * @param state lua state
 *    param 1: aabb x1
 *    param 2: aabb y1
 *    param 3: aabb x2
 *    param 4: aabb y2
 *    return hit count
 * @return 1 if hit, 0 if no hit
 */
int32_t queryAABB(lua_State* state)
{
}


/**
 * @brief queryRayCast do a raycast and see if we hit something
 * @param state lua state
 *    param 1 x1
 *    param 2 y1
 *    param 3 x2
 *    param 4 y2
 *    return number of objects hit
 * @return exit code
 */
int32_t queryRayCast(lua_State* state)
{
}


/**
 * @brief setDamage set the damage of this lua node
 * @param state lua state
 *    param damage amount of damage (0..100)
 * @return error code
 */
int32_t setDamage(lua_State* state)
{
}



/**
 * @brief setZ set the z layer of this node
 * @param state lua state
 *    param 1: z layer
 * @return exit code
 */
int32_t setZ(lua_State* state)
{
}


/**
 * @brief makeDynamic make this object a dynamic box2d object
 * @param state lua state
 * @return exit code
 */
int32_t makeDynamic(lua_State* state)
{
}


/**
 * @brief makeStatic make this object a static box2d object
 * @param state lua state
 * @return exit code
 */
int32_t makeStatic(lua_State* state)
{

}


/**
 * @brief setGravityScale set the gravity scale of this node
 * @param state lua state
 *    param 1: gravity scale (0..1)
 * @return error code
 */
int32_t setGravityScale(lua_State* state)
{
}


/**
 * @brief setActive set this node active/inactive
 * @param state lua state
 *    param 1: active flag
 * @return error code
 */
int32_t setActive(lua_State* state)
{
}


/**
 * @brief isPhsyicsPathClear check if a given path hits objects inside the tmx
 * @param state lua state
 *    param 1: x0
 *    param 2: y0
 *    param 3: x1
 *    param 4: y1
 *    return \c true on collision
 * @return error code
 */
int32_t isPhsyicsPathClear(lua_State* state)
{
}


/**
 * @brief getLinearVelocity reads the linear velocity of this object
 * @param state lua state
 *    return table
 *       1: velocity x
 *       2: velocity y
 * @return error code
 */
int32_t getLinearVelocity(lua_State* state)
{
}


/**
 * @brief setLinearVelocity setter for linear velocity
 * @param state lua state
 *    param 1: velocity x
 *    param 2: velocity y
 * @return error code
 */
int32_t setLinearVelocity(lua_State* state)
{
}


/**
 * @brief damage the node sets some damage to the player
 * @param state lua state
 *    param 1: amount of damage from 0..100
 *    param 2: dx damage direction x
 *    param 3: dy damage direction y
 * @return error code
 */
int32_t damage(lua_State* state)
{
}


/**
 * @brief damage the node damages the palyer if he's within a given radius
 * @param state lua state
 *    param 1: amount of damage from 0..100
 *    param 2: dx damage direction x
 *    param 3: dy damage direction y
 *    param 4: radius damage radius
 * @return error code
 */
int32_t damageRadius(lua_State* state)
{
}


/**
 * @brief setTransform set the object's transform
 * @param state lua state
 *    param 1: x translation
 *    param 2: y translation
 *    param 3: z rotation
 * @return error code
 */
int32_t setTransform(lua_State* state)
{
   // number of function arguments are on top of the stack.
   auto argc = lua_gettop(state);

   if (argc == 3)
   {
      auto x = static_cast<float>(lua_tonumber(state, 1));
      auto y = static_cast<float>(lua_tonumber(state, 2));
      auto angle = static_cast<float>(lua_tonumber(state, 3));

      std::shared_ptr<LuaNode> node = OBJINSTANCE;

      if (!node)
      {
         return 0;
      }

      b2Vec2 pos{x / PPM, y / PPM};
      node->setTransform(pos, angle);
   }

   return 0;
}


/**
 * @brief addSprite add another (empty) sprite to this node
 * @param state lua state
 * @return error code
 */
int32_t addSprite(lua_State* state)
{
   std::shared_ptr<LuaNode> node = OBJINSTANCE;

   if (!node)
   {
      return 0;
   }

   node->addSprite();

   return 0;
}


/**
 * @brief setSpriteOrigin set origin of a given sprite
 * @param state lua state
 *    param 1: sprite id
 *    param 2: x position
 *    param 3: y position
 * @return error code
 */
int32_t setSpriteOrigin(lua_State* state)
{
   // number of function arguments are on top of the stack.
   auto argc = lua_gettop(state);

   if (argc == 3)
   {
      auto id = static_cast<int32_t>(lua_tointeger(state, 1));
      auto x = static_cast<float>(lua_tonumber(state, 2));
      auto y = static_cast<float>(lua_tonumber(state, 3));

      std::shared_ptr<LuaNode> node = OBJINSTANCE;

      if (!node)
      {
         return 0;
      }

      node->setSpriteOrigin(id, x, y);
   }

   return 0;
}


/**
 * @brief setSpriteOffset sets the offset for a given sprite
 * @param state lua state
 *    param 1: sprite id
 *    param 2: x position
 *    param 3: y position
 * @return error code
 */
int32_t setSpriteOffset(lua_State* state)
{
   // number of function arguments are on top of the stack.
   auto argc = lua_gettop(state);

   if (argc == 3)
   {
      auto id = static_cast<int32_t>(lua_tointeger(state, 1));
      auto x = static_cast<float>(lua_tonumber(state, 2));
      auto y = static_cast<float>(lua_tonumber(state, 3));

      std::shared_ptr<LuaNode> node = OBJINSTANCE;

      if (!node)
      {
         return 0;
      }

      node->setSpriteOffset(id, x, y);
   }

   return 0;
}


/**
 * @brief boom make the game go booom
 * @param state lua state
 *    param 1: detonation center x
 *    param 2: detonation center y
 *    param 3: boom intensity
 * @return error code
 */
int32_t boom(lua_State* state)
{
   // number of function arguments are on top of the stack.
   auto argc = lua_gettop(state);

   if (argc == 3)
   {
      auto x = static_cast<float>(lua_tonumber(state, 1));
      auto y = static_cast<float>(lua_tonumber(state, 2));
      auto intensity = static_cast<float>(lua_tonumber(state, 3));

      std::shared_ptr<LuaNode> node = OBJINSTANCE;

      if (!node)
      {
         return 0;
      }

      node->boom(x, y, intensity);
   }

   return 0;
}


/**
 * @brief play a detonation animation
 * @param state lua state
 *    param 1: detonation center x
 *    param 2: detonation center y
 * @return error code
 */
int32_t playDetonationAnimation(lua_State* state)
{
   // number of function arguments are on top of the stack.
   auto argc = lua_gettop(state);

   if (argc == 2)
   {
      auto x = static_cast<float>(lua_tonumber(state, 1));
      auto y = static_cast<float>(lua_tonumber(state, 2));

      std::shared_ptr<LuaNode> node = OBJINSTANCE;

      if (!node)
      {
         return 0;
      }

      node->playDetonationAnimation(x, y);
   }

   return 0;
}


/**
 * @brief addShapeCircle add a circle shape to the node
 * @param state lua state
 *    param 1: circle radius
 *    param 2: circle x position
 *    param 3: circle y position
 * @return error code
 */
int32_t addShapeCircle(lua_State* state)
{
   auto argc = lua_gettop(state);

   if (argc == 3)
   {
      auto r = static_cast<float>(lua_tonumber(state, 1));
      auto x = static_cast<float>(lua_tonumber(state, 2));
      auto y = static_cast<float>(lua_tonumber(state, 3));

      std::shared_ptr<LuaNode> node = OBJINSTANCE;

      if (!node)
      {
         return 0;
      }

      node->addShapeCircle(r, x, y);
   }

   return 0;
}


/**
 * @brief addShapeRect add a rectangular shape to the node
 * @param state lua state
 *    param 1: rect width
 *    param 2: rect height
 *    param 3: rect position x
 *    param 4: rect position y
 * @return error code
 */
int32_t addShapeRect(lua_State* state)
{
   auto argc = lua_gettop(state);

   if (argc == 4)
   {
      auto width = static_cast<float>(lua_tonumber(state, 1));
      auto height = static_cast<float>(lua_tonumber(state, 2));
      auto x = static_cast<float>(lua_tonumber(state, 3));
      auto y = static_cast<float>(lua_tonumber(state, 4));

      std::shared_ptr<LuaNode> node = OBJINSTANCE;

      if (!node)
      {
         return 0;
      }

      node->addShapeRect(width, height, x, y);
   }

   return 0;
}


/**
 * @brief addShapePoly add a polygonal shape to the node
 * @param state lua state
 *    param n x coordinate
 *    param n + 1 y coordinate
 * @return error code
 */
int32_t addShapePoly(lua_State* state)
{
   auto argc = lua_gettop(state);

   if (argc >= 2 && (argc % 2 == 0))
   {
      auto size = argc / 2;
      b2Vec2* poly = new b2Vec2[static_cast<uint32_t>(size)];
      auto polyIndex = 0;
      for (auto i = 0; i < argc; i += 2)
      {
         auto x = static_cast<float>(lua_tonumber(state, i));
         auto y = static_cast<float>(lua_tonumber(state, i + 1));
         poly[polyIndex].Set(x, y);
         polyIndex++;
      }

      std::shared_ptr<LuaNode> node = OBJINSTANCE;

      if (!node)
      {
         delete[] poly;
         return 0;
      }

      node->addShapePoly(poly, size);
   }

   return 0;
}


/**
 * @brief addWeapon add a weapon instance to the player
 * @param state lua state
 *    param 1: weapon type (enum)
 *    param 2: fire interval in ms
 *    param 3: damage for single hit (0..100)
 *    param 4: bullet radius
 *    param 4..n: polygon x and y parameters if not a radial bullet
 * @return error code
 */
int32_t addWeapon(lua_State* state)
{
   auto argc = static_cast<size_t>(lua_gettop(state));

   if (argc < 3)
   {
      printf("bad parameters for addWeapon");
      exit(1);
   }

   WeaponType weapon_type = WeaponType::Default;
   auto fireInterval = 0;
   auto damage = 0;
   std::unique_ptr<b2Shape> shape;

   weapon_type = static_cast<WeaponType>(lua_tointeger(state, 1));
   fireInterval = static_cast<int>(lua_tointeger(state, 2));
   damage = static_cast<int>(lua_tointeger(state, 3));

   // add weapon with projectile radius only
   if (argc == 4)
   {
      auto radius = static_cast<float>(lua_tonumber(state, 4));
      shape = std::make_unique<b2CircleShape>();
      dynamic_cast<b2CircleShape*>(shape.get())->m_radius = radius;
   }

   // add weapon with polygon projectile shape
   if (argc >= 5 && ((argc - 5) % 2 == 0))
   {
      auto constexpr parameterCount = 2u;
      shape = std::make_unique<b2PolygonShape>();

      b2Vec2* poly = new b2Vec2[(argc - parameterCount) / 2];

      auto polyIndex = 0;
      for (auto i = parameterCount + 1; i < argc - parameterCount; i += 2u)
      {
         auto x = static_cast<float>(lua_tonumber(state, i));
         auto y = static_cast<float>(lua_tonumber(state, i + 1));
         poly[polyIndex].Set(x, y);
         polyIndex++;
      }

      dynamic_cast<b2PolygonShape*>(shape.get())->Set(poly, polyIndex);
   }

   std::shared_ptr<LuaNode> node = OBJINSTANCE;

   if (!node)
   {
      return 0;
   }

   auto weapon = WeaponFactory::create(node->mBody, weapon_type, std::move(shape), fireInterval, damage);
   node->addWeapon(std::move(weapon));

   return 0;
}


/**
 * @brief fireWeapon fire a weapon
 * @param state lua state
 *    param 1: index of the weapon
 *    param 2: x position where the shot comes from
 *    param 3: y position where the shot comes from
 *    param 4: x direction
 *    param 5: y direction
 * @return error code
 */
int32_t fireWeapon(lua_State* state)
{
   auto argc = lua_gettop(state);

   if (argc == 5)
   {
      auto index = static_cast<size_t>(lua_tointeger(state, 1));

      auto posX = static_cast<float>(lua_tonumber(state, 2)) * MPP;
      auto posY = static_cast<float>(lua_tonumber(state, 3)) * MPP;

      auto dirX = static_cast<float>(lua_tonumber(state, 4));
      auto dirY = static_cast<float>(lua_tonumber(state, 5));

      std::shared_ptr<LuaNode> node = OBJINSTANCE;

      if (!node)
      {
         return 0;
      }

      node->fireWeapon(index, {posX, posY}, {dirX, dirY});
   }

   return 0;
}


/**
 * @brief updateProjectileTexture change the texture of a projectile
 * @param state lua state
 *    param 1: index of the weapon
 *    param 2: path of the texture
 *    param 3: x position of the texture rect
 *    param 4: y position of the texture rect
 *    param 5: width of the texture rect
 *    param 6: height of the texture rect
 * @return error code
 */
int32_t updateProjectileTexture(lua_State* state)
{
   auto argc = lua_gettop(state);
   auto valid = (argc >= 2);

   auto index = 0u;
   std::string path;

   if (valid)
   {
      index = static_cast<uint32_t>(lua_tointeger(state, 1));
      path = lua_tostring(state, 2);
   }

   sf::Rect<int32_t> rect;
   if (argc == 6)
   {
      auto x1 = static_cast<int32_t>(lua_tointeger(state, 3));
      auto y1 = static_cast<int32_t>(lua_tointeger(state, 4));

      auto width = static_cast<int32_t>(lua_tointeger(state, 5));
      auto height = static_cast<int32_t>(lua_tointeger(state, 6));

      rect.left = x1;
      rect.top = y1;
      rect.width = width;
      rect.height = height;
   }

   if (valid)
   {
      std::shared_ptr<LuaNode> node = OBJINSTANCE;

      if (!node)
      {
         return 0;
      }
      const auto& texture = TexturePool::getInstance().get(path);
      node->mWeapons[index]->setProjectileAnimation(texture, rect);
   }

   return 0;
}


/**
 * @brief updateProjectileAnimation set projectile animation for a given weapon
 * @param state lua state
 *    param 1: weapon index
 *    param 2: texture path
 *    param 3: width of one frame
 *    param 4: height of one frame
 *    param 5: x origin of the frame
 *    param 6: y origin of the frame
 *    param 7: time for each frame in seconds
 *    param 8: frame count
 *    param 9: frames per row
 *    param 10: start frame
 * @return error code
 */
int32_t updateProjectileAnimation(lua_State* state)
{
   int32_t argc = lua_gettop(state);

   if (argc == 10)
   {
      auto weapon_index          = static_cast<uint32_t>(lua_tointeger(state, 1));
      std::filesystem::path path = lua_tostring(state, 2);
      auto frame_width           = static_cast<uint32_t>(lua_tointeger(state, 3));
      auto frame_height          = static_cast<uint32_t>(lua_tointeger(state, 4));
      auto frame_origin_x        = static_cast<float>(lua_tointeger(state, 5));
      auto frame_origin_y        = static_cast<float>(lua_tointeger(state, 6));
      auto time_per_frame_s      = static_cast<float>(lua_tonumber(state, 7));
      auto frame_count           = static_cast<uint32_t>(lua_tointeger(state, 8));
      auto frames_per_row        = static_cast<uint32_t>(lua_tointeger(state, 9));
      auto start_frame           = static_cast<uint32_t>(lua_tointeger(state, 10));

      std::shared_ptr<LuaNode> node = OBJINSTANCE;

      if (!node)
      {
         return 0;
      }

      auto texture = TexturePool::getInstance().get(path);

      sf::Vector2f frame_origin{frame_origin_x, frame_origin_y};

      // assume identical frame times for now
      std::vector<sf::Time> frame_times;
      for (auto i = 0u; i < frame_count; i++)
      {
         frame_times.push_back(sf::seconds(time_per_frame_s));
      }

      AnimationFrameData frame_data(
         texture,
         frame_origin,
         frame_width,
         frame_height,
         frame_count,
         frames_per_row,
         frame_times,
         start_frame
      );

      node->mWeapons[weapon_index]->setProjectileAnimation(frame_data);
   }

   return 0;
}


/**
 * @brief timer start a timer
 * @param state lua state
 *    param 1: delay of the timer
 *    param 2: id of the timer in milliseconds
 * @return error code
 */
int32_t timer(lua_State* state)
{
   // number of function arguments are on top of the stack.
   auto argc = lua_gettop(state);

   if (argc == 2)
   {
      auto delay = static_cast<int32_t>(lua_tointeger(state, 1));
      auto timerId = static_cast<int32_t>(lua_tointeger(state, 2));
      std::shared_ptr<LuaNode> node = OBJINSTANCE;

      if (!node)
      {
         return 0;
      }

      Timer::add(
         std::chrono::milliseconds(delay),
         [node, timerId](){node->luaTimeout(timerId);},
         Timer::Type::Singleshot,
         nullptr,
         node
      );
   }

   return 0;
}


/**
 * @brief addSample add a sample to be played later
 * @param state lua state
 *    param 1: name of the sample
 * @return error code
 */
int32_t addSample(lua_State* state)
{
   // number of function arguments are on top of the stack.
   int32_t argc = lua_gettop(state);

   if (argc == 1)
   {
      auto sample = lua_tostring(state, 1);
      Audio::getInstance()->addSample(sample);
   }

   return 0;
}


/**
 * @brief playSample play a sample
 * @param state lua state
 *    param 1: name of the sample to play
 *    param 2: volume (0..1)
 * @return
 */
int32_t playSample(lua_State* state)
{
   // number of function arguments are on top of the stack.
   int32_t argc = lua_gettop(state);

   if (argc == 2)
   {
      auto sample = lua_tostring(state, 1);
      auto volume = static_cast<float>(lua_tonumber(state, 2));

      Audio::getInstance()->playSample(sample, volume);
   }

   return 0;
}


/**
 * @brief debug output a debug message to stdout
 * @param state lua state
 *    param 1: debug message
 * @return error code
 */
int32_t debug(lua_State* state)
{
   // number of function arguments are on top of the stack.
   int32_t argc = lua_gettop(state);

   if (argc == 1)
   {
      const char* message = lua_tostring(state, 1);
      puts(message);
   }

   return 0;
}


/**
 * @brief registerHitAnimation register a hit animation for a given weapon
 * @param state lua state
 *    param 1: weapon index
 *    param 2: texture path
 *    param 3: width of one frame
 *    param 4: height of one frame
 *    param 5: frame count
 *    param 6: frames per row
 *    param 7: start frame
 * @return error code
 */
int32_t registerHitAnimation(lua_State* state)
{
   int32_t argc = lua_gettop(state);

   if (argc == 8)
   {
      auto weapon_index          = static_cast<uint32_t>(lua_tointeger(state, 1));
      std::filesystem::path path = lua_tostring(state, 2);
      auto frame_width           = static_cast<uint32_t>(lua_tointeger(state, 3));
      auto frame_height          = static_cast<uint32_t>(lua_tointeger(state, 4));
      auto time_per_frame_s      = static_cast<float>(lua_tonumber(state, 5));
      auto frame_count           = static_cast<uint32_t>(lua_tointeger(state, 6));
      auto frames_per_row        = static_cast<uint32_t>(lua_tointeger(state, 7));
      auto start_frame           = static_cast<uint32_t>(lua_tointeger(state, 8));

      ProjectileHitAnimation::addReferenceAnimation(
         path,
         frame_width,
         frame_height,
         std::chrono::duration<float, std::chrono::seconds::period>{time_per_frame_s},
         frame_count,
         frames_per_row,
         start_frame
      );

      std::shared_ptr<LuaNode> node = OBJINSTANCE;

      if (!node)
      {
         return 0;
      }

      node->mWeapons[weapon_index]->setProjectileIdentifier(path.string());
   }

   return 0;
}


/**
 * @brief updateKeysPressed fire keypressed events to the node instance
 * @param state lua state
 *    param 1: keypressed bitmask
 * @return error code
 */
int32_t updateKeysPressed(lua_State* state)
{
   auto argc = lua_gettop(state);

   if (argc == 1)
   {
      auto keyPressed = static_cast<int32_t>(lua_tointeger(state, 1));

      auto obj = LuaInterface::instance()->getObject(state);
      if (obj != nullptr)
      {
         LuaInterface::instance()->updateKeysPressed(obj, keyPressed);
      }
   }

   return 0;
}


/**
 * @brief requestMap request the game map
 * @param state lua state
 * @return error code
 */
int32_t requestMap(lua_State* state)
{
   auto obj = LuaInterface::instance()->getObject(state);
   if (obj != nullptr)
   {
      LuaInterface::instance()->requestMap(obj);
   }

   return 0;
}


/**
 * @brief die let the node die
 * @param state lua state
 * @return error code
 */
int32_t die(lua_State* state)
{
   std::shared_ptr<LuaNode> node = OBJINSTANCE;

   if (!node)
   {
      return 0;
   }

   node->luaDie();
   return 0;
}


[[noreturn]] void error(lua_State* state, const char* /*scope*/ = nullptr)
{
  // the error message is on top of the stack.
  // fetch it, print32_t it and then pop it off the stack.
   std::stringstream os;
   os << lua_tostring(state, -1);

   std::cout << os.str() << std::endl;

   lua_pop(state, 1);

   exit(1);
}


```