------------------------------------------------------------------------------------------------------------------------
require "data/scripts/enemies/constants"
v2d = require "data/scripts/enemies/vectorial2"

------------------------------------------------------------------------------------------------------------------------
properties = {
   staticBody = true,
   sprite = "data/sprites/enemy_bonefish.png",
   damage = 200
}


------------------------------------------------------------------------------------------------------------------------
mPosition = v2d.Vector2D(0, 0)
mPlayerPosition = v2d.Vector2D(0, 0)
mSpriteIndex = 0
mElapsed = 0.0
mCycle = 0
mPatrolTimer = 1
mKeyPressed = 0


------------------------------------------------------------------------------------------------------------------------
function initialize()

   patrolPath = {}
   patrolIndex = 1
   patrolEpsilon = 1.0

   addShapeRect(0.2, 0.2, -0.05, 0.0)
   updateSpriteRect(0, 0 * 48, 48, 24) -- x, y, width, height
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
end


------------------------------------------------------------------------------------------------------------------------
function goRight()
end


------------------------------------------------------------------------------------------------------------------------
function patrol()
   if (wait == true) then
      return
   end

   local key = patrolPath[patrolIndex]
   local keyVec = v2d.Vector2D(key:getX(), key:getY())
   local count = #patrolPath

   if     (mPosition:getX() > keyVec:getX() + patrolEpsilon) then
      goLeft()
   elseif (mPosition:getX() < keyVec:getX() - patrolEpsilon) then
      goRight()
   else
      -- print("arrived.")
      wait = true
      mKeyPressed = 0
      timer(500, mPatrolTimer)
      patrolIndex = patrolIndex + 1
      if (patrolIndex > count) then
         patrolIndex = 0
      end
   end
end

------------------------------------------------------------------------------------------------------------------------
function update(dt)
   -- setGravityScale(-0.0025)
   patrol()
end


------------------------------------------------------------------------------------------------------------------------
function retrieveProperties()
   updateProperties(properties)
end


------------------------------------------------------------------------------------------------------------------------
function movedTo(x, y)
   mPosition = v2d.Vector2D(x, y)
end


------------------------------------------------------------------------------------------------------------------------
function playerMovedTo(x, y)
   mPlayerPosition = v2d.Vector2D(x, y)
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
      patrolPath = v
   end
end


------------------------------------------------------------------------------------------------------------------------
function timeout(id)
   -- print(string.format("timeout: %d", id))
   if (id == mPatrolTimer) then
      wait = false
   end
end
