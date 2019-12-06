------------------------------------------------------------------------------------------------------------------------
require "data/scripts/enemies/constants"
v2d = require "data/scripts/enemies/vectorial2"


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
   sprite = "data/sprites/enemy_bat.png",
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
function cosineInterpolate(y1, y2, mu)
   mu2 = (1 - math.cos(mu * math.pi)) * 0.5
   val = (y1 * (1 - mu2) + y2 * mu2)
   -- print(string.format("y1: %f, y2: %f, mu: %f -> %f", y1, y2, mu, val))
   return val
end


------------------------------------------------------------------------------------------------------------------------
function cubicInterpolate(y0, y1, y2, y3, mu)
   mu2 = mu * mu
   a0 = y3 - y2 - y0 + y1
   a1 = y0 - y1 - a0
   a2 = y2 - y0
   a3 = y1

   return (a0 * mu * mu2 + a1 * mu2 + a2 * mu + a3)
end


------------------------------------------------------------------------------------------------------------------------
function tableLength(T)
   local count = 0
   for _ in pairs(T) do count = count + 1 end
   return count
end


------------------------------------------------------------------------------------------------------------------------
function findIndex(track, time)
   size = 0
   for k, v in pairs(track) do
      if (v.time > time) then
         return k - 1
      end
         size = size +1
   end
   -- reached end of table
   return size
end


------------------------------------------------------------------------------------------------------------------------
function getValueCos(track, time)
   value = 0.0
   y1i = findIndex(track, time)
   size = tableLength(track)

   -- clamp
   if (y1i < 1) then
      p = v2d.Vector2D(track[1].x, track[1].y)
      return p
   elseif (y1i >= size) then
      p = v2d.Vector2D(track[size].x, track[size].y)
      return p
   else
      y2i = y1i + 1
   end

   -- do cosine interpolation
   y1 = track[y1i]
   y2 = track[y2i]

   range = y2.time - y1.time
   mu = (time - y1.time) / range

   x = cosineInterpolate(y1.x, y2.x, mu)
   y = cosineInterpolate(y1.y, y2.y, mu)
   p = v2d.Vector2D(x, y)

   return p
end

------------------------------------------------------------------------------------------------------------------------
function getValueCubic(track, time)
   value = 0.0

   y1i = findIndex(track, time)
   y0i = y1i - 1
   y2i = y1i + 1
   y3i = y1i + 2

   size = tableLength(track)

   y0i = math.max(1, y0i)
   y1i = math.max(1, y1i)
   y2i = math.max(1, y2i)
   y3i = math.max(1, y3i)

   y0i = math.min(size, y0i)
   y1i = math.min(size, y1i)
   y2i = math.min(size, y2i)
   y3i = math.min(size, y3i)

   y0 = track[y0i]
   y1 = track[y1i]
   y2 = track[y2i]
   y3 = track[y3i]

   range = y2.time - y1.time

   if (range < 0.01) then
      return v2d.Vector2D(y1.x, y1.y)
   end

   mu = (time - y1.time) / range
   x = cubicInterpolate(y0.x, y1.x, y2.x, y3.x, mu)
   y = cubicInterpolate(y0.y, y1.y, y2.y, y3.y, mu)
   p = v2d.Vector2D(x, y)

   return p
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

   sx = mStartPosition:getX()
   sy = mStartPosition:getY()

   k1 = Key:create{x = bx,          y = by,             time = 0.0}
   k2 = Key:create{x = px - 1 * 24, y = py - 1.75 * 24, time = 0.2} -- left of player
   k3 = Key:create{x = px,          y = py - 1.5 * 24,  time = 0.5} -- player pos
   k4 = Key:create{x = px + 1 * 24, y = py - 1.75 * 24, time = 0.8} -- right of player
   k5 = Key:create{x = sx,          y = sy,             time = 1.0} -- go back

   mPath = {k1, k2, k3, k4, k5}
end


------------------------------------------------------------------------------------------------------------------------
function update(dt)

   updateSprite = false

   -- get sprite index
   mElapsed = mElapsed + dt

   idle = mElapsed < mIdleTime

   -- make sure block is on same x as player
   if (not mAttack or idle) then
      xDiff = mPosition:getX() // 24 - mPlayerPosition:getX() // 24
      if (math.abs(xDiff) < 3) then

         -- make sure stone is not too far away (10 tiles) and above player
         yDiff = mPosition:getY() // 24 - mPlayerPosition:getY() // 24

         if (yDiff < 0 and yDiff > -10 and not idle) then
            attack()
         end
      end

      -- update transform
      mTransformY = 0.25 * math.sin(mElapsed) * mMoveRangeY
      setTransform(mStartPosition:getX(), mStartPosition:getY() + mTransformY, 0.0)
   else
      time = (mElapsed - mAttackTime) / 3.5
      p = getValueCubic(mPath, time)
      -- p = getValueCos(mPath, time)
      -- print(string.format("i: %f, x: %f, y: %f", time, p:getX(), p:getY()))
      setTransform(p:getX(), p:getY(), 0.0)

      if (time > 1.0) then
         mAttack = false
         mIdleTime = mElapsed + 3
      end
   end

   spriteIndex = math.floor(math.fmod(mElapsed * 4.0, 3))

   -- update sprite index
   if (index ~= mSpriteOffsetX) then
      mSpriteOffsetX = spriteIndex
      updateSprite = true
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


