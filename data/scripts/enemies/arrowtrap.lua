require "data/scripts/enemies/constants"
v2d = require "data/scripts/enemies/vectorial2"

-- arrow rows:
--    points left:  5
--    points right: 6
--    points down:  7
--    points up:    8
--
-- start animation:
--    column 0..8
--
-- arrow fly
--    column 9..12
--
-- arrow rotate
--    column 13..18
--
-- arrow fade out
--    column 19..23


------------------------------------------------------------------------------------------------------------------------
properties = {
   staticBody = true,
   sprite = "data/sprites/enemy_arrowtrap.png",
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
mSpeed = 1.5

mIdle = true

SPRITE_WIDTH = 3 * 24
SPRITE_HEIGHT = 3 * 24


------------------------------------------------------------------------------------------------------------------------
function getOffset(alignment)
   offset = v2d.Vector2D(0, 0)

   if (alignment == Alignment["AlignmentUp"]) then
      offset:setX(5)
      offset:setY(1)
   elseif (alignment == Alignment["AlignmentDown"]) then
      offset:setX(5)
      offset:setY(0)
   elseif (alignment == Alignment["AlignmentLeft"]) then
      offset:setX(0)
      offset:setY(0)
   elseif (alignment == Alignment["AlignmentRight"]) then
      offset:setX(0)
      offset:setY(1)
   end

   return offset
end


-- physical box is just 24 * 24px
-- pixel box is 3 * 24 * 3 * 24px
--
--   +---+---+---+
--   |   |   |   |
--   +---+---+---+
--   |   |///|   |
--   +---+---+---+
--   |   |   |   |
--   +---+---+---+
--


------------------------------------------------------------------------------------------------------------------------
function initialize()

   addShapeRect(0.25, 0.25, 0.25, 0.25)
   addSample("boom.wav")
   addWeapon(WeaponType["Bow"], 1000, 60, 0.1) -- interval, damage, radius
   updateProjectileTexture(0, "data/weapons/arrow.png", 144, 984, 48, 24) -- index, path, x, y, width, height

   alignment = Alignment["AlignmentRight"]

   offset = getOffset(alignment)

   updateSpriteRect(
      0,
      offset:getX() * SPRITE_WIDTH,
      offset:getY() * SPRITE_HEIGHT,
      SPRITE_WIDTH,
      SPRITE_HEIGHT
   )

   setSpriteOrigin(0, -12, -12)

end


------------------------------------------------------------------------------------------------------------------------
function writeProperty(key, value)

   print(string.format("write property: %s %s", key, value))

   config[key] = value

   if (key == "alignment") then
      if (value == "right") then
         print("setting alignment to left")
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

   mSpriteIndex = math.floor(mElapsed * 20.0)

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

      -- fire the ball at the right sprite index
      if (col == 7 and row == 0) then
         fire()
      end

      -- animation is done, go to idle animation
      if (col == 7 and row == 2) then
         mIdle = true
      end
   end

   -- print(string.format("col: %f, row: %f", col, row))

   -- if (index ~= mSpriteIndex) then
   --    updateSpriteRect(
   --       0,
   --       col * SPRITE_WIDTH,
   --       row * SPRITE_HEIGHT + mAlignmentOffset,
   --       SPRITE_WIDTH,
   --       SPRITE_HEIGHT
   --    )
   -- end
end


------------------------------------------------------------------------------------------------------------------------
function setPath(name, table)
end

