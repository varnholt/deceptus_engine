------------------------------------------------------------------------------------------------------------------------
require "data/scripts/enemies/constants"
require "data/scripts/enemies/interpolation"
v2d = require "data/scripts/enemies/vectorial2"


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
   damage = 30
}


------------------------------------------------------------------------------------------------------------------------
mPosition = v2d.Vector2D(0, 0)
mPlayerPosition = v2d.Vector2D(0, 0)
mElapsed = math.random(0, 3)
mPointsRight = false

mAttack = false
mAttackTime = 0
mAttackPath = {}
mAttackSpeed = 0.4
mAttackDistanceFactor = 1.0

mIdle = false
mIdleTime = 0
mIdlePosition = v2d.Vector2D(0, 0)

mMoveHome = false
mMoveHomeTime = 0
mMoveHomePath = {}
mMoveHomeSpeed = 0.6

mPatrol = true
mPatrolTime = 0
mPatrolPath = {}

mMoveRangeY = 48
mSpriteOffsetX = 0
mSpriteOffsetY = 0
mSpriteSize = 48
mStartPosition = v2d.Vector2D(0, 0)
mDone = false
mTransformY = 0
mUpdateSprite = false


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
function updateSpriteDir(pointsRight)
   pointsRightOld = mPointsRight
   mPointsRight = pointsRight
   mUpdateSprite = (mPointsRight ~= pointsRightOld)
end


------------------------------------------------------------------------------------------------------------------------
function updateSprite(src, dst)
   updateSpriteDir(dst > src)
end


------------------------------------------------------------------------------------------------------------------------
function startAttack()

   print("start attack")

   mAttack = true
   mAttackTime = mElapsed

   bx = mPosition:getX()
   by = mPosition:getY()

   px = mPlayerPosition:getX()
   py = mPlayerPosition:getY() - 24

   k1 = Key:create{x = bx, y = by, time = 0.0}
   k2 = Key:create{x = px, y = py, time = 1.0}

   updateSprite(bx, px)

   ax = px - bx
   ay = py - by
   dist = math.sqrt(ax * ax + ay * ay)
   mAttackDistanceFactor = 200.0 / dist

   -- print(mAttackDistanceFactor)

   mAttackPath = {k1, k2}
end


------------------------------------------------------------------------------------------------------------------------
function startMoveHome()

   print("start move home")

   mMoveHome = true
   mMoveHomeTime = mElapsed

   -- current position
   bx = mPosition:getX()
   by = mPosition:getY()

   -- where we came from
   sx = mStartPosition:getX()
   sy = mStartPosition:getY()

   k1 = Key:create{x = bx, y = by, time = 0.0}
   k2 = Key:create{x = sx, y = sy, time = 1.0}

   updateSprite(bx, sx)

   mMoveHomePath = {k1, k2}
end


------------------------------------------------------------------------------------------------------------------------
function startIdle()
   print("start idle")
   mIdle = true
   mIdleTime = mElapsed
   mIdlePosition = mPosition
end


------------------------------------------------------------------------------------------------------------------------
function startPatrol()
   print("start patrol")
   mPatrol = true
   mPatrolTime = mElapsed
end


------------------------------------------------------------------------------------------------------------------------
function idle()

   time = (mElapsed - mIdleTime)
   mTransformY = 0.25 * math.sin(time) * mMoveRangeY
   setTransform(mIdlePosition:getX(), mIdlePosition:getY() + mTransformY, 0.0)

   if (time > 1.0) then
      print("stop idle")
      mIdle = false
   end
end


------------------------------------------------------------------------------------------------------------------------
function moveHome()

   time = (mElapsed - mMoveHomeTime) * mMoveHomeSpeed
   p = getValueCos(mMoveHomePath, time)

   setTransform(p:getX(), p:getY(), 0.0)

   if (time > 1.0) then
      print("stop move home")
      mMoveHome = false
      startPatrol()
   end
end


------------------------------------------------------------------------------------------------------------------------
function attack()
   time = (mElapsed - mAttackTime) * mAttackSpeed * mAttackDistanceFactor
   p = getValueCos(mAttackPath, time)

   setTransform(p:getX(), p:getY(), 0.0)

   -- attack move finished, need to locate player again
   if (time > 1.0) then
      mAttack = false
      print("stop attack")
      startIdle()
   end
end


------------------------------------------------------------------------------------------------------------------------
function patrol()

   distanceToHomeX = mPosition:getX() // 24 - mStartPosition:getX() // 24

   -- is ghost too far away from home, then go home
   if (math.abs(distanceToHomeX) > 10) then
      print("too far away from home, return home")
      startMoveHome()
      return
   end

   -- check if player is within range
   distanceToPlayerX = mPosition:getX() // 24 - mPlayerPosition:getX() // 24

   if (math.abs(distanceToPlayerX) < 10) then

      distanceToPlayerY = mPosition:getY() // 24 - mPlayerPosition:getY() // 24

      if (math.abs(distanceToPlayerY) < 5) then
         mPatrol = false
         print("player in range")
         startAttack()
         return
      end
   end

   -- if player is too far away, then go home
   if (not mPatrol) then
      startMoveHome()
   end

   -- after moving home, start patrol cycle
   if (mPatrol) then
      time = mElapsed - mPatrolTime
      timeVal = math.fmod(time * 0.25, 1.0)
      updateSpriteDir(timeVal < 0.5) -- values from 0 .. 0.5 go to right
      p = getValueCos(mPatrolPath, timeVal)
      mTransformY = 0.25 * math.sin(time) * mMoveRangeY
      setTransform(p:getX(), mStartPosition:getY() + mTransformY, 0.0)
   end
end


------------------------------------------------------------------------------------------------------------------------
function update(dt)

   -- get sprite index
   mElapsed = mElapsed + dt

   if (mIdle) then
      idle()
   elseif (mMoveHome) then     -- if player is out of sight after idle, go home
      moveHome()
   elseif (mAttack) then       -- if player is in sight after idle, attack
      attack()
   else
      patrol()                 -- moved home, just patrol and locate player
   end

   -- update sprite
   if (mUpdateSprite) then
      updateSpriteRect(
         mSpriteOffsetX * mSpriteSize,
         (mPointsRight and mSpriteSize or 0),
         mSpriteSize,
         mSpriteSize
      ) -- x, y, width, height
   end

   -- die
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
         -- print(string.format("v%d: %f, %f", (i - 1) / 2, x, y))
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

