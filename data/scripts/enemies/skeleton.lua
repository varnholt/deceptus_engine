require "data/scripts/enemies/constants"
require "data/scripts/enemies/helpers"
v2d = require "data/scripts/enemies/vectorial2"

-- enemy configuration
properties = {
   sprite = "data/sprites/enemy_skeleton.png",
   velocity_walk_max = 1.0,
   acceleration_ground = 0.1
}

------------------------------------------------------------------------------------------------------------------------
mPatrolTimer = 1
mKeyPressed = 0
mPosition = v2d.Vector2D(0, 0)
mPlayerPosition = v2d.Vector2D(0, 0)
mPointsLeft = false
mElapsed = math.random(0, 3)
mSpriteIndex = 0

-- x: 720..792 (30..33 x 24)
-- y: 984 (41 x 24)


------------------------------------------------------------------------------------------------------------------------
function initialize()
   patrolPath = {}
   patrolIndex = 1
   patrolEpsilon = 1.0
   wait = false
   addShapeRect(0.2, 0.5, 0.0, 0.23)
   updateSpriteRect(0, 0, 72, 72)

   -- print("dumb.lua initialized")
end


------------------------------------------------------------------------------------------------------------------------
function timeout(id)
   -- print(string.format("timeout: %d", id))
   if (id == mPatrolTimer) then
      wait = false
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
   mPointsLeft = true
   keyReleased(Key["KeyRight"])
   keyPressed(Key["KeyLeft"])
end


------------------------------------------------------------------------------------------------------------------------
function goRight()
   mPointsLeft = false
   keyReleased(Key["KeyLeft"])
   keyPressed(Key["KeyRight"])
end


------------------------------------------------------------------------------------------------------------------------
function setPatrolPositions(x1, y1, x2, y2)
end


------------------------------------------------------------------------------------------------------------------------
function movedTo(x, y)
   -- print(string.format("moved to: %f, %f", x, y))
   mPosition = v2d.Vector2D(x, y)
end


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
      timer(3000, mPatrolTimer)
      patrolIndex = patrolIndex + 1
      if (patrolIndex > count) then
         patrolIndex = 0
      end
   end
end


------------------------------------------------------------------------------------------------------------------------
function updateAttackCondition()
end


------------------------------------------------------------------------------------------------------------------------
function update(dt)

   mElapsed = mElapsed + dt
   spriteCount = 12
   updateSprite = false
   pointsLeftPrev = mPointsLeft
   mAnimationOffset = 0

   -- as in: if skeleton isn't idle
   if (mKeyPressed == 0) then
      mAnimationOffset = 144
      spriteCount = 10
   end

   spriteIndex = math.floor(math.fmod(mElapsed * 15.0, spriteCount))

   patrol()
   updateAttackCondition()
   updateKeysPressed(mKeyPressed)

   -- update sprite index
   if (spriteIndex ~= mSpriteIndex) then
      mSpriteIndex = spriteIndex
      updateSprite = true
   end

   if (pointsLeftPrev ~= mPointsLeft) then
      updateSprite = true
   end

   if (updateSprite) then
      updateSpriteRect(
         mSpriteIndex * 72,
         mAnimationOffset + (mPointsLeft and 72 or 0),
         72,
         72
      ) -- x, y, width, height
   end

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



