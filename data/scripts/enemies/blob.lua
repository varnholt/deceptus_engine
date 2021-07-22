require "data/scripts/enemies/constants"
v2d = require "data/scripts/enemies/vectorial2"


------------------------------------------------------------------------------------------------------------------------
properties = {
   sprite = "data/sprites/deprecated_enemy_blob.png",
   velocity_walk_max = 1.0,
   acceleration_ground = 0.1,
   damage = 40
}


------------------------------------------------------------------------------------------------------------------------
mPatrolTimer = 1
mKeyPressed = 0

mPosition = v2d.Vector2D(0, 0)
mPlayerPosition = v2d.Vector2D(0, 0)


-- x: 720..792 (30..33 x 24)
-- y: 984 (41 x 24)


------------------------------------------------------------------------------------------------------------------------
function initialize()
   mPatrolPath = {}
   mPatrolIndex = 1
   mPatrolEpsilon = 1.0
   mWait = false

   addShapeCircle(0.12, 0.0, 0.12)   -- radius, x, y
   addShapeRect(0.2, 0.07, 0.0, 0.1) -- width, height, x, y
   updateSpriteRect(0, 0, 0, 24, 24) -- id, x, y, width, height

   -- print("blob.lua initialized")
end


------------------------------------------------------------------------------------------------------------------------
function timeout(id)
   -- print(string.format("timeout: %d", id))
   if (id == mPatrolTimer) then
      mWait = false
   end
end


------------------------------------------------------------------------------------------------------------------------
function retrieveProperties()
   updateProperties(properties)
end


------------------------------------------------------------------------------------------------------------------------
function keyPressed(key)
   mKeyPressed = (mKeyPressed | key)
end


------------------------------------------------------------------------------------------------------------------------
function keyReleased(key)
   mKeyPressed = mKeyPressed & (~key)
end


------------------------------------------------------------------------------------------------------------------------
function goLeft()
   updateSpriteRect(0, 0, 0, 24, 24)
   keyReleased(Key["KeyRight"])
   keyPressed(Key["KeyLeft"])
end


------------------------------------------------------------------------------------------------------------------------
function goRight()
   updateSpriteRect(0, 0, 24, 24, 24)
   keyReleased(Key["KeyLeft"])
   keyPressed(Key["KeyRight"])
end


------------------------------------------------------------------------------------------------------------------------
function movedTo(x, y)
   -- print(string.format("moved to: %f, %f", x, y))
   mPosition = v2d.Vector2D(x, y)
end


------------------------------------------------------------------------------------------------------------------------
function playerMovedTo(x, y)
   -- print(string.format("player moved to: %f, %f", x, y))
   mPlayerPosition = v2d.Vector2D(x, y)
end


------------------------------------------------------------------------------------------------------------------------
-- |          o      p          | x
-- |          p      o          | x
function followPlayer()
   local epsilon = 5
   if (mPlayerPosition:getX() > mPosition:getX() + epsilon) then
      goRight()
   elseif (mPlayerPosition:getX() < mPosition:getX() - epsilon) then
      goLeft()
   else
      mKeyPressed = 0
   end
end


------------------------------------------------------------------------------------------------------------------------
function patrol()
   if (mWait == true) then
      return
   end

   local key = mPatrolPath[mPatrolIndex]
   local keyVec = v2d.Vector2D(key:getX(), key:getY())
   local count = #mPatrolPath

   if     (mPosition:getX() > keyVec:getX() + mPatrolEpsilon) then
      goLeft()
   elseif (mPosition:getX() < keyVec:getX() - mPatrolEpsilon) then
      goRight()
   else
      -- print("arrived.")
      mWait = true
      mKeyPressed = 0
      timer(500, mPatrolTimer)
      mPatrolIndex = mPatrolIndex + 1
      if (mPatrolIndex > count) then
         mPatrolIndex = 0
      end
   end
end


------------------------------------------------------------------------------------------------------------------------
function update(dt)
   patrol()
   updateKeysPressed(mKeyPressed)
end


------------------------------------------------------------------------------------------------------------------------
function setPath(name, table)
   -- print(string.format("Received %d arguments:", #table))

   local i = 0
   local x = 0.0;
   local y = 0.0;
   local v = {}

   for key, value in pairs(table) do

      if ((i % 2) == 0) then
         x = value
      else
         y = value
         -- print(string.format("v%d: %f, %f", (i - 1) / 2, x, y))
         v[(i - 1) / 2] = v2d.Vector2D(x, y)
      end

      i = i + 1
   end

   if (name == "patrol_path") then
      mPatrolPath = v
   end
end



