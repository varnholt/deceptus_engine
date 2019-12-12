------------------------------------------------------------------------------------------------------------------------
require "data/scripts/enemies/constants"
require "data/scripts/enemies/interpolation"
v2d = require "data/scripts/enemies/vectorial2"


-- +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
-- |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
-- +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
-- |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
-- +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
-- |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
-- +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
-- |   |   |   |   |###|###|###|###|###|###|###|###|###|###|###|###|###|   |   |   |   |
-- +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
-- |   |   |   |   |###|###|###|###|###|###|###|###|###|###|###|###|###|   |   |   |   |
-- +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
-- |   |   |   |   |###|###|###|###|###|###|###|###|###|###|###|###|###|   |   |   |   |
-- +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
-- |   |   |   |   |###|###|###|###|###|###|###|###|###|###|###|###|###|   |   |   |   |
-- +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
-- |P00|P01|   |   |<<<|###|###|###|###|###|###|###|###|###|###|###|>>>|   |   |   |   |
-- +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
--  <<<             <<< patrol path start           patrol path end >>>             >>>
--  extended range                                                       extended range


-- 221, 77
-- 210, 78
-- 223, 77
-- player: 206, 82

------------------------------------------------------------------------------------------------------------------------
-- interpolation keys
Key = {x = 0, y = 0, time = 0}
function Key:create(o)
  o.parent = self
  return o
end


------------------------------------------------------------------------------------------------------------------------
properties = {
   staticBody = true,
   sprite = "data/sprites/enemy_ghost.png",
   damage = 0
}


------------------------------------------------------------------------------------------------------------------------
mPosition = v2d.Vector2D(0, 0)
mPlayerPosition = v2d.Vector2D(0, 0)
mElapsed = math.random(0, 3)
mAttackTime = 0
mIdleTime = 0
mActivated = false
mMoveRangeY = 48
mSpriteOffsetX = 0
mSpriteOffsetY = 0
mSpriteSize = 48
mStartPosition = v2d.Vector2D(0, 0)
mDone = false
mTransformY = 0
mAttack = false
mPath = {}
mPatrolPath = {}
mFindPlayerInterval = 1000
mFindPlayerTimer = 0
mFindTimerStart = true


------------------------------------------------------------------------------------------------------------------------
function initialize()
   addShapeCircle(0.2, 0.0, 0.0)
   updateSpriteRect(
      0,
      0,
      mSpriteSize,
      mSpriteSize
   )
   setZ(30) -- somewhere in the foreground
end


------------------------------------------------------------------------------------------------------------------------
function attack()
   mAttack = true
   mAttackTime = mElapsed

   print("attack")

   bx = mPosition:getX()
   by = mPosition:getY()

   px = mPlayerPosition:getX()
   py = mPlayerPosition:getY()

   k1 = Key:create{x = bx, y = by, time = 0.0}
   k2 = Key:create{x = px, y = py, time = 1.0}

   mPath = {k1, k2}
end


------------------------------------------------------------------------------------------------------------------------
function backToStart()
   bx = mPosition:getX()
   by = mPosition:getY()

   sx = mStartPosition:getX()
   sy = mStartPosition:getY()

   k1 = Key:create{x = bx, y = by, time = 0.0}
   k2 = Key:create{x = sx, y = sy, time = 1.0}

   mPath = {k1, k2}
end


------------------------------------------------------------------------------------------------------------------------
function update(dt)

   updateSprite = false

   -- get sprite index
   mElapsed = mElapsed + dt

   idle = mElapsed < mIdleTime

   -- locate player
   if (mFindTimerStart) then
      mFindTimerStart = false
      timer(mFindPlayerInterval, mFindPlayerTimer)
   end

   -- idle
   if (not mAttack or idle) then
      p = getValueCos(mPatrolPath, math.fmod(mElapsed * 0.25, 1.0))
      mTransformY = 0.25 * math.sin(mElapsed) * mMoveRangeY
      setTransform(p:getX(), mStartPosition:getY() + mTransformY, 0.0)

   -- attack player
   else
      time = (mElapsed - mAttackTime) / 3.5
      p = getValueCos(mPath, time)
      -- p = getValueCubic(mPath, time)
      -- print(string.format("i: %f, x: %f, y: %f", time, p:getX(), p:getY()))

      setTransform(p:getX(), p:getY(), 0.0)

      -- attack move finished, need to locate player again
      if (time > 1.0) then
         mAttack = false

         -- actually there is no need to idle, could remove this code
         mIdleTime = mElapsed + 3
      end
   end

   if (updateSprite) then
      updateSpriteRect(
         mSpriteOffsetX * mSpriteSize,
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
         print(string.format("v%d: %f, %f", (i - 1) / 2, x, y))
         v[(i - 1) / 2] = v2d.Vector2D(x, y)
      end

      i = i + 1
   end

   if (name == "patrol_path") then

      k1 = Key:create{x = v[0]:getX(), y = v[0]:getY(), time = 0.0}
      k2 = Key:create{x = v[1]:getX(), y = v[1]:getY(), time = 0.5}
      k3 = Key:create{x = v[0]:getX(), y = v[0]:getY(), time = 1.0}

      mPatrolPath = {k1, k2, k3}
   end
end



------------------------------------------------------------------------------------------------------------------------
function retrieveProperties()
   updateProperties(properties)
end


------------------------------------------------------------------------------------------------------------------------
function setStartPosition(x, y)
   -- print(string.format("object position: %f, %f", x, y))
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
function locatePlayer()

   xDiff = mPosition:getX() // 24 - mPlayerPosition:getX() // 24

   if (math.abs(xDiff) < 10) then

       yDiff = mPosition:getY() // 24 - mPlayerPosition:getY() // 24

       if (yDiff < 0 and math.abs(yDiff) < 5) then
          print("player in range")
          attack()
       else
          print("player out of range, go home")
          backToStart()
       end
    end
end



------------------------------------------------------------------------------------------------------------------------
function timeout(id)
   print(string.format("timeout: %d", id))
   if (id == mFindPlayerTimer) then
      mFindTimerStart = true
      locatePlayer()
   end
end
