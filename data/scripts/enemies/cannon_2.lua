
require "data/scripts/enemies/constants"
v2d = require "data/scripts/enemies/vectorial2"

-- place cannons here
-- 64,311
-- 78,311

------------------------------------------------------------------------------------------------------------------------
properties = {
   staticBody = true,
   sprite = "data/sprites/enemy_pirate_cannon.png",
   damage = 0
}


------------------------------------------------------------------------------------------------------------------------
mStartFireTimer = 1
mFireInterval = 3000
mReadytoFire = true
mPosition = v2d.Vector2D(0, 0)
mPlayerPosition = v2d.Vector2D(0, 0)
mSpriteIndex = 0
mElapsed = 0.0
mFireElapsed = 0.0
mX = -1.0
mAlignmentOffset = 0
mSpeed = 1.5
mProjectileIndex = 0

mIdle = true
mFired = false

SPRITE_WIDTH = 6 * 24
SPRITE_HEIGHT = 3 * 24


------------------------------------------------------------------------------------------------------------------------
function initialize()
   addShapeRect(0.4, 0.2, 0.0, 0.0) -- width, height, x, y
   addSample("boom.wav")

   addWeapon(WeaponType["Gun"], 1000, 60, 0.2) -- interval, damage, radius

   registerHitAnimation(
      0,
      "data/sprites/enemy_pirate_cannon_cannonball.png",
      3 * 24,
      3 * 24,
      0.1,
      20,
      24,
      4
   )

   updateProjectileAnimation(
      0,
      "data/sprites/enemy_pirate_cannon_cannonball.png",
      3 * 24,
      3 * 24,
      (3 * 24) / 2,
      (3 * 24) / 2,
      0.1,
      4,
      15,
      0
   )

   setSpriteOffset(0, 0, -24);
end


------------------------------------------------------------------------------------------------------------------------
function writeProperty(key, value)
   if (key == "alignment") then
      if (value == "right") then
         -- print("setting alignment to left")
         mX = 1.0
         mAlignmentOffset = 5 * SPRITE_HEIGHT
      end
   end
end


------------------------------------------------------------------------------------------------------------------------
function timeout(id)
   -- print(string.format("timeout: %d", id))

   if (id == mStartFireTimer) then
      mReadytoFire = true
   end

end


------------------------------------------------------------------------------------------------------------------------
function fire()
   useGun(
      0,
      mPosition:getX() + mX * 16,
      mPosition:getY() - 12,
      mX * mSpeed,
      0.0
   );
end


------------------------------------------------------------------------------------------------------------------------
function retrieveProperties()
   updateProperties(properties)
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
function update(dt)

   -- if timer elapsed, go from idle animation to fire animation
   if (mReadytoFire) then
      mReadytoFire = false
      timer(mFireInterval, mStartFireTimer)
      mIdle = false
      mElapsed = 0
   end

   -- update sprite index
   mElapsed = mElapsed + dt
   index = mSpriteIndex
   mSpriteIndex = math.floor(mElapsed * 20.0)

   -- update projectile index
   projectileIndex = 0
   if (mFired) then
      mFireElapsed = mFireElapsed + dt
      projectileIndex = math.floor(mFireElapsed * 5.0)
      projectileIndex = math.min(3, projectileIndex)
   end

   col = 0
   row = 0

   -- idle animation
   if (mIdle) then
      col = mSpriteIndex % 3
      row = 0

   -- fire animation
   else
      col = mSpriteIndex % 12
      row = 1 + (math.floor(mSpriteIndex / 12)) % 4

      -- fire the ball at the right sprite index
      if (col == 5 and row == 2) then
         fire()
         mFired = true
      end

      -- update projectile index after the cannon has been fired
      if (mFired) then

         if (projectileIndex ~= mProjectileIndex) then

            mProjectileIndex = projectileIndex

            -- this is the wrong function to call since we want to alter the fired projectile
            -- updateProjectileTexture(
            --    0,
            --    "data/sprites/enemy_pirate_cannon_cannonball.png",
            --    mProjectileIndex * 72,
            --    0,
            --    72,
            --    72
            -- )

         end
      end

      -- animation is done, go to idle animation
      if (col == 3 and row == 4) then
         mIdle = true
         mFired = false
         mFireElapsed = 0
      end
   end

   -- print(string.format("col: %f, row: %f", col, row))

   if (index ~= mSpriteIndex) then
      updateSpriteRect(
         0,
         col * SPRITE_WIDTH,
         row * SPRITE_HEIGHT + mAlignmentOffset,
         SPRITE_WIDTH,
         SPRITE_HEIGHT
      )
   end

end


------------------------------------------------------------------------------------------------------------------------
function setPath(name, table)
end

