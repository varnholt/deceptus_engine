require "data/scripts/enemies/constants"
v2d = require "data/scripts/enemies/vectorial2"

-- one tile = 3x3x24
--
-- +---+---+---+
-- |   |   |   |
-- +---+---+---+
-- |   | x |   |
-- +---+---+---+
-- |   |   |   |
-- +---+---+---+
-- 0 ..    .. 72


------------------------------------------------------------------------------------------------------------------------
properties = {
   staticBody = true,
   sprite = "data/sprites/landmine.png",
   pointsUp = Alignment["AlignmentUp"],
   damage = 0
}


------------------------------------------------------------------------------------------------------------------------
mTriggered = false
mExploded = false
mDone = false
mDetonationTimer = 1
mPosition = v2d.Vector2D(0, 0)
mPlayerPosition = v2d.Vector2D(0, 0)
mElapsed = 0
mSpriteCol = -1
mSpriteRow = -1
mSpriteSize = 72
mExplosionFrame = 0
mExplosionStrength = 5.0
mDamageDistance = 40.0;
mDamage = 100.0;


------------------------------------------------------------------------------------------------------------------------
function updateSprite(dt)
   col = 0
   row = 1

   if (not mTriggered and not mExploded) then
      if ((math.floor(mElapsed)) % 2 == 0) then
         col = 0
      else
         col = 1
      end
   elseif (mTriggered and not mExploded) then
      if ((math.floor(mElapsed * 4)) % 2 == 0) then
         col = 3
      else
         col = 2
      end
   elseif (mExploded) then
      mExplosionFrame = mExplosionFrame + dt * 30.0
      if (mExplosionFrame >= 10.0) then
         mDone = true
      end
      row = 0
      col = math.floor(mExplosionFrame)
   elseif (mDone) then
      row = 1
      col = 4
   end

   if (col ~= mSpriteCol or row ~= mSpriteRow) then
      updateSpriteRect(
         col * mSpriteSize,
         row * mSpriteSize,
         mSpriteSize,
         mSpriteSize
      )
      mSpriteCol = col
      mSpriteRow = row
   end
end


------------------------------------------------------------------------------------------------------------------------
function initialize()
   addShapeRect(0.2, 0.2, 0.0, 0.1)
   updateSprite(0.0)
   -- print("landmine.lua initialized")
end


------------------------------------------------------------------------------------------------------------------------
function timeout(id)
   if (id == mDetonationTimer) then
      mExploded = true
      playSample(Sample["SampleBoom"], 1.0)
      boom(mPosition:getX(), mPosition:getY(), 1.0)

      distanceToPlayer = mPlayerPosition - mPosition;
      distanceToPlayerLength = (distanceToPlayer):getLength()
      if (distanceToPlayerLength < mDamageDistance) then
        damageVal = mDamage * (mDamageDistance - distanceToPlayerLength) / mDamageDistance;
        damage(0, damageVal, mExplosionStrength * distanceToPlayer:getX(), -mExplosionStrength * distanceToPlayer:getY())
      end

      die()
   end
end


------------------------------------------------------------------------------------------------------------------------
function retrieveProperties()
   updateProperties(properties)
end


------------------------------------------------------------------------------------------------------------------------
function movedTo(x, y)
   -- print(string.format("object position: %f, %f", x, y))
   mPosition = v2d.Vector2D(x, y)
end


------------------------------------------------------------------------------------------------------------------------
function playerMovedTo(x, y)
   mPlayerPosition = v2d.Vector2D(x, y)
   distanceToPlayer = (mPlayerPosition - mPosition):getLength()

   if (not mTriggered and distanceToPlayer < 20.0) then
      mTriggered = true
      timer(2000, mDetonationTimer)
   end
end


------------------------------------------------------------------------------------------------------------------------
function update(dt)
   mElapsed = mElapsed + dt
   updateSprite(dt)
end


------------------------------------------------------------------------------------------------------------------------
function setPath(name, table)
end

