------------------------------------------------------------------------------------------------------------------------
require "data/scripts/enemies/constants"
v2d = require "data/scripts/enemies/vectorial2"


------------------------------------------------------------------------------------------------------------------------
properties = {
   staticBody = true,
   sprite = "data/sprites/enemy_watermine.png",
   damage = 0
}


------------------------------------------------------------------------------------------------------------------------
mPosition = v2d.Vector2D(0, 0)
mPlayerPosition = v2d.Vector2D(0, 0)
mElapsed = math.random(0, 3)
mActivated = false
mSpriteIndex = 0
mDetonationTimer = 1
mMoveRangeY = 48
mSpriteOffsetY = 24
mSpriteSize = 48
mStartPosition = v2d.Vector2D(0, 0)


------------------------------------------------------------------------------------------------------------------------
function initialize()
   addShapeCircle(0.27, 0.0, 0.0)
   updateSpriteRect(
      0,
      mSpriteOffsetY,
      mSpriteSize,
      mSpriteSize
   )
end


------------------------------------------------------------------------------------------------------------------------
function update(dt)
   updateSprite = false

   -- get sprite index
   mElapsed = mElapsed + dt

   if (not mActivated) then
      spriteIndex = math.floor(math.fmod(mElapsed * 2.0, 3))
   else
      spriteIndex = 2 + math.floor(math.fmod(mElapsed * 4.0, 2))
   end

   -- get sprite direction
   y = 0.5 * math.sin(mElapsed) * mMoveRangeY

   -- update transform
   setTransform(mStartPosition:getX(), mStartPosition:getY() + y, 0.0)

   -- update sprite index
   if (index ~= mSpriteIndex) then
      mSpriteIndex = spriteIndex
      updateSprite = true
   end

   if (updateSprite) then
      updateSpriteRect(
         mSpriteIndex * mSpriteSize,
         mSpriteOffsetY,
         mSpriteSize,
         mSpriteSize
      ) -- x, y, width, height
   end
end



------------------------------------------------------------------------------------------------------------------------
function collisionWithPlayer()
   if (not mActivated) then
      print("collision with player!")
      mActivated = true
      timer(5000, mDetonationTimer)
   end
end


------------------------------------------------------------------------------------------------------------------------
function setPath(name, table)
end


------------------------------------------------------------------------------------------------------------------------
function retrieveProperties()
   updateProperties(properties)
end


------------------------------------------------------------------------------------------------------------------------
function setStartPosition(x, y)
   print(string.format("object position: %f, %f", x, y))
   mStartPosition = v2d.Vector2D(x, y)
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
function timeout(id)
   if (id == mDetonationTimer) then
      print("boom.")
   end
end

