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
mActivated = false
mSpriteIndex = 0
mMoveRangeY = 48
mSpriteOffsetY = 24
mSpriteSize = 48
mStartPosition = v2d.Vector2D(0, 0)
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
function cosineInterpolate(y1, y2, mu)
   mu2 = (1 - math.cos(mu * math.pi)) / 2
   val = (y1 * (1 - mu2) + y2 * mu2)
   -- print(string.format("y1: %f, y2: %f, mu: %f -> %f", y1, y2, mu, val))
   return val
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
function getValue(track, time)
   value = 0.0
   y1i = findIndex(track, time)
   size = tableLength(track)

   -- clamp
   if (y1i < 1) then
      y1i = 1
   elseif (y1i > size) then
      y1i = size
      y2i = size
   else
      y2i = y1i + 1
   end

   y1 = track[y1i]
   y2 = track[y2i]

   range = y2.time - y1.time
   mu = (time - y1.time) / range

   x = cosineInterpolate(y1.x, y2.x, mu)
   y = cosineInterpolate(y1.x, y2.x, mu)
   p = v2d.Vector2D(x, y)

   return p
end


------------------------------------------------------------------------------------------------------------------------
function computePath()
   print("start")

   k1 = Key:create{x =  0, y =  0, time = 0.0}
   k2 = Key:create{x =  5, y =  5, time = 0.1}
   k3 = Key:create{x = -5, y = -5, time = 0.6}
   k4 = Key:create{x =  0, y =  0, time = 1.0}

   track = {k1, k2, k3, k4}

   for i = 0, 1, 0.01 do
      p = getValue(track, i)
      print(string.format("i: %f, x: %f, y: %f", i, p:getX(), p:getY()))
   end

   print("done")
end


------------------------------------------------------------------------------------------------------------------------
function update(dt)
   updateSprite = false

   -- get sprite index
   mElapsed = mElapsed + dt

   if (not mActivated) then
      spriteIndex = math.floor(math.fmod(mElapsed * 2.0, 3))
   end

   -- get sprite direction
   mTransformY = 0.5 * math.sin(mElapsed) * mMoveRangeY

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


