
require "data/scripts/enemies/constants"
require "data/scripts/enemies/helpers"
v2d = require "data/scripts/enemies/vectorial2"

-- enemy configuration
properties = {
   sprite = "data/sprites/deprecated_enemy_endboss_1.png",
   damage = 100
}


------------------------------------------------------------------------------------------------------------------------
mKeyPressed = 0
mPosition = v2d.Vector2D(0, 0)
mPlayerPosition = v2d.Vector2D(0, 0)
mStartPosition = v2d.Vector2D(0, 0)
mElapsed = 0
mFirePause = 0
mSinking = false
mYPrev = 0.0


------------------------------------------------------------------------------------------------------------------------
function initialize()
   wait = false

   addShapeRect(0.6, 1.05, 0.25, 0.0)
   updateSpriteRect(0, 0, 0, 80, 104)

   addWeapon(WeaponType["Gun"], 250, 80, 0,0, 0,0.1666, 1.0,0.1666, 1.0,0, 0,0) -- interval, damage, radius/shape
   updateProjectileTexture(0, "data/sprites/deprecated_enemy_endboss_1.png", 0, 112, 49, 8) -- index, path, x, y, width, height
end


------------------------------------------------------------------------------------------------------------------------
function timeout(id)
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
function movedTo(x, y)
   mPosition = v2d.Vector2D(x, y)

   if (mPosition:getY() > 6917) then
      die()
   end
end


------------------------------------------------------------------------------------------------------------------------
function playerMovedTo(x, y)
   mPlayerPosition = v2d.Vector2D(x, y)
end


------------------------------------------------------------------------------------------------------------------------
function updateFire()

   useGun(
      0,
      mPosition:getX() - 80,
      mPosition:getY() - 9,
      -3.0,
      (math.random(0, 50) * 0.08 - 2.0)
   );

end


------------------------------------------------------------------------------------------------------------------------
function updateFirePause(dt)

   mFirePause = mFirePause + dt

   if (mFirePause > 8) then
      mFirePause = 0
   end

end


------------------------------------------------------------------------------------------------------------------------
function update(dt)

   mElapsed = mElapsed + dt

   -- update transform
   if (not mSinking) then

      y = mStartPosition:getY() + 104 - math.abs(math.sin(mElapsed * 4.0)) * 100;

      setTransform(
         mStartPosition:getX(),
         y,
         0.0
      )

      if (mFirePause < 3 or mFirePause > 5) then
         updateFire()
      end

      updateFirePause(dt)

      -- check if elevator is moving down
      queryCount = queryRayCast(7347, 6666, 7347, 6679)

      if (queryCount == 0) then

         -- since the endboss is just moved a long a fixed path, it's required to
         -- pass a linear velocity once box2d will do the position handling again
         setLinearVelocity(0.0, y - mYPrev)
         mSinking = true
      end

      mYPrev = y

   end
end


------------------------------------------------------------------------------------------------------------------------
function setStartPosition(x, y)
   mStartPosition = v2d.Vector2D(x, y)
end


