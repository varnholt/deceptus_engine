------------------------------------------------------------------------------------------------------------------------
require "data/scripts/enemies/constants"
v2d = require "data/scripts/enemies/vectorial2"


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


-- https://repl.it/repls/TraumaticPartialOctagon
print("start")

Key = {x = 0, y = 0, time = 0}

function Key:create(o)
  o.parent = self
  return o
end

function findIndex(track, time)
  for k, v in pairs(track) do 
    if (v.time > time) then
       return k - 1
    end
  end
end

function cosineInterpolate(y1, y2, mu)
   mu2 = (1 - math.cos(mu * math.pi))/2
   return (y1 * (1 - mu2) + y2 * mu2)
end 

function getValue(track, time)
  value = 0.0
  index = findIndex(track, time)

  y1 = track[index]
  y2 = track[index+1]

  mu = 0.0

  value = cosineInterpolate(y1.x, y2.x, mu)

  return value
end


k1 = Key:create{x =  0, y =  0, time = 0.0}
k2 = Key:create{x =  5, y =  5, time = 0.1}
k3 = Key:create{x = -5, y = -5, time = 0.6}
k4 = Key:create{x =  0, y =  0, time = 1.0}

track = {k1, k2, k3, k4}


-- indices start at 1

-- for k,v in pairs(track) do 
--   print(v.x)
-- end

print(getValue(track, 0.7))
print("done")


------------------------------------------------------------------------------------------------------------------------
function update(dt)
   updateSprite = false

   -- get sprite index
   mElapsed = mElapsed + dt

   if (not mActivated) then
      spriteIndex = math.floor(math.fmod(mElapsed * 2.0, 3))

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


