require "data/scripts/enemies/constants"
require "data/scripts/enemies/helpers"
v2d = require "data/scripts/enemies/vectorial2"

-- enemy configuration
properties = {
   sprite = "data/sprites/rat.png",
}


------------------------------------------------------------------------------------------------------------------------
mPatrolTimer = 1
mPosition = v2d.Vector2D(0, 0)
mPlayerPosition = v2d.Vector2D(0, 0)
mPointsToLeft = false


------------------------------------------------------------------------------------------------------------------------
function initialize()
   patrolPath = {}
   patrolIndex = 1
   patrolEpsilon = 1.0
   wait = false
   updateSpriteRect(0, 0, 0, 64, 64)
end


------------------------------------------------------------------------------------------------------------------------
function timeout(id)
   if (id == mPatrolTimer) then
      wait = false
   end
end


------------------------------------------------------------------------------------------------------------------------
function retrieveProperties()
   updateProperties(properties)
end


------------------------------------------------------------------------------------------------------------------------
function goLeft()
   mPointsToLeft = true
   updateSpriteRect(0, 0, 64, 64, 64)
end


------------------------------------------------------------------------------------------------------------------------
function goRight()
   mPointsToLeft = false
   updateSpriteRect(0, 0, 0, 64, 64)
end


------------------------------------------------------------------------------------------------------------------------
function playerMovedTo(x, y)
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
   if (wait == true) then
      return
   end

   local key = patrolPath[patrolIndex]
   local keyVec = v2d.Vector2D(key:getX(), key:getY())
   local count = #patrolPath

   if (mPosition:getX() > keyVec:getX() + patrolEpsilon) then
      goLeft()
   elseif (mPosition:getX() < keyVec:getX() - patrolEpsilon) then
      goRight()
   else
      wait = true
      mKeyPressed = 0
      timer(5000, mPatrolTimer)
      patrolIndex = patrolIndex + 1
      if (patrolIndex > count) then
         patrolIndex = 0
      end
   end
end


------------------------------------------------------------------------------------------------------------------------
function update(dt)
   patrol()
end


------------------------------------------------------------------------------------------------------------------------
function setPath(name, table)

   local i = 0
   local x = 0.0;
   local y = 0.0;
   local v = {}

   for key, value in pairs(table) do

      if ((i % 2) == 0) then
         x = value
      else
         y = value
         v[(i - 1) / 2] = v2d.Vector2D(x, y)
      end

      i = i + 1
   end

   if (name == "path") then
      patrolPath = v
   end
end

