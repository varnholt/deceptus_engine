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
mDetonationElapsed = 0
mActivated = false
mSpriteIndex = 0
mDetonationTimer = 1
mMoveRangeY = 48
mSpriteOffsetY = 24
mSpriteSize = 48
mStartPosition = v2d.Vector2D(0, 0)
mDetonating = false
mDone = false
mTransformY = 0


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

   elseif (mActivated and not mDetonating) then
      spriteIndex = 2 + math.floor(math.fmod(mElapsed * 4.0, 2))

   elseif (mDetonating) then
      mSpriteOffsetY = 24 + 48
      mDetonationElapsed = mDetonationElapsed + dt
      spriteIndex = math.floor(mDetonationElapsed * 15.0)

      if (spriteIndex > 6) then
         mDone = true
      end
   end

   -- get sprite direction
   if (not mDetonating) then
      mTransformY = 0.5 * math.sin(mElapsed) * mMoveRangeY
   end

   -- update transform
   setTransform(mStartPosition:getX(), mStartPosition:getY() + mTransformY, 0.0)

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

   if (mDone) then
      die()
   end

end



------------------------------------------------------------------------------------------------------------------------
function collisionWithPlayer()
   if (not mActivated) then
      print("collision with player!")
      mActivated = true
      timer(2000, mDetonationTimer)
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
      mDetonating = true
      playSample("boom.wav", 1.0)
      damageRadius(200, mPosition:getX(), mPosition:getY(), 48)
   end
end


