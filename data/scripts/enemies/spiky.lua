-- row 0: idle left
-- row 1: idle right
-- row 2: walk left
-- row 3: walk right
-- row 4: hide left
-- row 5: hide right
-- row 6, 7: idle left
-- row 8, 9: idle right
--
-- hide left: 48-59
-- hide right: 60-71
-- reveal left: 51-48
-- reveal right: 63-60

-- 12 sprites per row

-- size is 2 x 24, 2 x 24
-- origin is in the bottom center
-- +---+---+
-- | ##|## |
-- +---+---+
-- | ##|## |
-- +---+---+

require "data/scripts/enemies/constants"
require "data/scripts/enemies/helpers"
v2d = require "data/scripts/enemies/vectorial2"

-- enemy configuration
properties = {
   sprite = "data/sprites/enemy_spiky.png",
   velocity_walk_max = 1.0,
   acceleration_ground = 0.1
}


------------------------------------------------------------------------------------------------------------------------
mKeyPressed = 0
mPosition = v2d.Vector2D(0, 0)
mPlayerPosition = v2d.Vector2D(0, 0)
mPointsToLeft = false


------------------------------------------------------------------------------------------------------------------------
function initialize()
   patrolPath = {}
   patrolIndex = 1
   patrolEpsilon = 1.0
   wait = false

   addShapeCircle(0.24, 0.0, 0.24)
   -- addShapeRect(0.2, 0.07, 0.0, 0.1)

   updateSpriteRect(0, 0, 0, 48, 48)
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
   mPointsToLeft = true
   updateSpriteRect(0, 0, 64, 64, 64)
   keyReleased(Key["KeyRight"])
   keyPressed(Key["KeyLeft"])
end


------------------------------------------------------------------------------------------------------------------------
function goRight()
   mPointsToLeft = false
   updateSpriteRect(0, 0, 0, 64, 64)
   keyReleased(Key["KeyLeft"])
   keyPressed(Key["KeyRight"])
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

   if     (mPosition:getX() > keyVec:getX() + patrolEpsilon) then
      goLeft()
   elseif (mPosition:getX() < keyVec:getX() - patrolEpsilon) then
      goRight()
   else
      wait = true
      mKeyPressed = 0

--      timer(5000, mPatrolTimer)
--      patrolIndex = patrolIndex + 1
--      if (patrolIndex > count) then
--         patrolIndex = 0
--      end
   end
end


------------------------------------------------------------------------------------------------------------------------
function update(dt)
   -- patrol()
   updateKeysPressed(mKeyPressed)
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



