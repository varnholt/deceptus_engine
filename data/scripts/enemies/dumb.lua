require "data/scripts/enemies/constants"
require "data/scripts/enemies/helpers"
v2d = require "data/scripts/enemies/vectorial2"

-- enemy configuration
properties = {
   sprite = "data/sprites/enemy_test.png",
   velocity_walk_max = 1.0,
   acceleration_ground = 0.1
}

------------------------------------------------------------------------------------------------------------------------
mPatrolTimer = 1
mKeyPressed = 0
mPosition = v2d.Vector2D(0, 0)
mPlayerPosition = v2d.Vector2D(0, 0)
mPointsToLeft = false
mFireDistance = 300


-- x: 720..792 (30..33 x 24)
-- y: 984 (41 x 24)


------------------------------------------------------------------------------------------------------------------------
function initialize()
   patrolPath = {}
   patrolIndex = 1
   patrolEpsilon = 1.0
   wait = false
   addShapeCircle(0.12, 0.0, 0.12)
   addShapeRect(0.2, 0.07, 0.0, 0.1)
   updateSpriteRect(0, 0, 64, 64)

   addWeapon(100, 0.05) -- interval, radius
   updateBulletTexture(0, "data/sprites/orb.png", 0, 0, 24, 24) -- index, path, x, y, width, height

   print("dumb.lua initialized")
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
   mPointsToLeft = true
   updateSpriteRect(0, 64, 64, 64)
   keyReleased(Key["KeyRight"])
   keyPressed(Key["KeyLeft"])
end


------------------------------------------------------------------------------------------------------------------------
function goRight()
   mPointsToLeft = false
   updateSpriteRect(0, 0, 64, 64)
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
      timer(5000, mPatrolTimer)
      patrolIndex = patrolIndex + 1
      if (patrolIndex > count) then
         patrolIndex = 0
      end
   end
end


------------------------------------------------------------------------------------------------------------------------
function updateShootCondition()

   if (math.abs(mPosition:getY() - mPlayerPosition:getY()) < 10) then
      if (math.abs(mPosition:getX() - mPlayerPosition:getX()) < mFireDistance) then

         playerIsLeft = (mPosition:getX() > mPlayerPosition:getX())

         openFire = false
         fireDir = 1.0
         if (playerIsLeft and mPointsToLeft) then
            openFire = true
            fireDir = -1.0
         elseif (not playerIsLeft and not mPointsToLeft) then
            openFire = true
         end

         if (isPhsyicsPathClear(mPosition:getX(), mPosition:getY(), mPosition:getX() + fireDir * mFireDistance, mPosition:getY()))
         then
            if (openFire) then
               fireWeapon(0, mPosition:getX() + (fireDir * 32), mPosition:getY(), fireDir * 0.05, 0.0);
            end
         end
      end
   end

end


------------------------------------------------------------------------------------------------------------------------
function update(dt)
   patrol()
   updateShootCondition()
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
      patrolPath = v
   end
end



