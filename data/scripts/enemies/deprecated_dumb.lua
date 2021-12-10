require "data/scripts/enemies/constants"
require "data/scripts/enemies/helpers"
v2d = require "data/scripts/enemies/vectorial2"

-- enemy configuration
properties = {
   sprite = "data/sprites/deprecated_enemy_test.png",
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


------------------------------------------------------------------------------------------------------------------------
function initialize()
   patrolPath = {}
   patrolIndex = 1
   patrolEpsilon = 1.0
   wait = false
   addShapeCircle(0.12, 0.0, 0.12)
   addShapeRect(0.2, 0.07, 0.0, 0.1)
   updateSpriteRect(0, 0, 0, 64, 64)

   addWeapon(WeaponType["Gun"], 300, 10, 0.1) -- interval, damage, radius
   updateProjectileTexture(0, "data/sprites/orb.png", 0, 0, 24, 24) -- index, path, x, y, width, height
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

         if (
            isPhsyicsPathClear(
               mPosition:getX(),
               mPosition:getY(),
               mPlayerPosition:getX(),
               mPlayerPosition:getY()
            )
         )
         then
            if (openFire) then
               useGun(
                  0,
                  mPosition:getX() + (fireDir * 32),
                  mPosition:getY(),
                  fireDir * 4.0,
                  0.0
               );
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



