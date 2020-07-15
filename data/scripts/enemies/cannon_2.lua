
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
config = {
}

------------------------------------------------------------------------------------------------------------------------
mStartFireTimer = 1
mFireInterval = 3000
mReadytoFire = true
mPosition = v2d.Vector2D(0, 0)
mPlayerPosition = v2d.Vector2D(0, 0)
mSpriteIndex = 0
mElapsed = 0.0
mX = -1.0
mAlignmentOffset = 0
mSpeed = 1.5

mIdle = true

SPRITE_WIDTH = 6 * 24
SPRITE_HEIGHT = 5 * 24


------------------------------------------------------------------------------------------------------------------------
function initialize()
   addShapeRect(0.2, 0.2, 0.0, 0.1) -- width, height, x, y
   addSample("boom.wav")
   addWeapon(1000, 60, 0.1) -- interval, damage, radius
   updateBulletTexture(0, "data/sprites/enemy_pirate_cannon.png", 144, 984, 48, 24) -- index, path, x, y, width, height
end


------------------------------------------------------------------------------------------------------------------------
function writeProperty(key, value)
   config[key] = value

   if (key == "alignment") then
      if (value == "right") then
         -- print("setting alignment to left")
         mX = 1.0
         mAlignmentOffset = 4 * SPRITE_HEIGHT
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
   fireWeapon(
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

   mSpriteIndex = math.floor(mElapsed * 10.0)

   col = 0
   row = 0

   -- idle animation
   if (mIdle) then
      col = mSpriteIndex % 8
      row = 3

   -- fire animation
   else
      col = mSpriteIndex % 8
      row = (math.floor(mSpriteIndex / 8)) % 3

      -- fire the bullet at the right sprite index
      if (col == 7 and row == 0) then
         fire()
      end

      -- animation is done, go to idle animation
      if (col == 7 and row == 2) then
         mIdle = true
      end
   end

   -- print(string.format("col: %f, row: %f", col, row))

   if (index ~= mSpriteIndex) then
      updateSpriteRect(
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

