
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
   sprite = "data/sprites/enemy_critter.png",
   damage = 30
}


--
-- mode 1: line
--
-- +---+---+---+---+---+---+---+
-- |   |p_0|   |   |   |   |   |
-- +---+---+---+---+---+---+---+
--      x--------> |   |p_1|   |
--               | +---+---+---+
--               +--------->
--
--
-- mode 2: loop
--
--  +--------------------------
--  |  +---+---+---+---+---+  /\
--  |  |t_0|   |   |   |t_4|  |
-- \/  +---+---+---+---+---+  |
--  ---------+ |   |   |   |  |
--           | +---+---+---+  |
--           +----------------+


------------------------------------------------------------------------------------------------------------------------
mPosition = v2d.Vector2D(0, 0)
mElapsed = math.random(0, 100)
mSpriteIndex = 0
mPointsLeft = true
mSpeed = 1.0

EPSILON = 0.0001
DISTANCE_FROM_PATH_PX = 10


------------------------------------------------------------------------------------------------------------------------
function initialize()

   mPatrolPath = {}

   addShapeRect(0.2, 0.2, -0.05, 0.0)
   updateSpriteRect(0, 0, 0, 48, 48)
end


------------------------------------------------------------------------------------------------------------------------

-- cw
-- 0: right, horizontal, up
-- 3: left, horizontal, down
-- 4: right, vertical, down
-- 7: left, vertical, up

-- ccw
-- 2: right, horizontal, down
-- 1: left, horizontal, up
-- 5: right, vertical, up
-- 6: left, vertical, down

--
-- clockwise
-- 0 -> 4 -> 3 -> 7
--
-- counter clockwise
-- 1 -> 6 -> 2 -> 5
function updateSprite(p)

   if (mPosition == nil) then
      return
   end

   dx = (p:getX() - mPosition:getX())
   dy = (p:getY() - mPosition:getY())

   row = 0

   if (mPointsLeft) then
      if (dx > EPSILON) then
         row = 2
      elseif  (dx < -EPSILON) then
         row = 1
      end

      if (dy > EPSILON) then
         row = 6
      elseif (dy < -EPSILON) then
         row = 5
      end
   else
      if (dx > EPSILON) then
         row = 0
      elseif  (dx < -EPSILON) then
         row = 3
      end

      if (dy > EPSILON) then
         row = 4
      elseif (dy < -EPSILON) then
         row = 7
      end
   end

   updateSpriteRect(0, 0, row * 48, 48, 48)
end


------------------------------------------------------------------------------------------------------------------------
function update(dt)

   if (#mPatrolPath == 0) then
      return
   end

   mElapsed = mElapsed + dt

   val = math.fmod(mSpeed * mElapsed * 0.2, 1.0)
   p = getValueLinear(mPatrolPath, val);

   setTransform(p:getX(), p:getY(), 0.0)
   updateSprite(p)

   mPosition = p

end


------------------------------------------------------------------------------------------------------------------------
function setPath(name, table)
   -- print(string.format("Received %s, %d arguments:", name, #table))

   local i = 0
   local x = 0.0;
   local y = 0.0;
   local path = {}

   for key, value in pairs(table) do

      if ((i % 2) == 0) then
         x = value
      else
         y = value
         index = (i - 1) / 2
         -- print(string.format("v%d: %f, %f", index, x, y))
         path[index] = v2d.Vector2D(x, y)
      end

      i = i + 1
   end

   if (name == "path") then

      -- need to scale each path, so it increases in 24px width and 24px height
      minX = 9999
      maxX = 0
      minY = 9999
      maxY = 0

      for key, value in pairs(path) do
         x = value:getX()
         y = value:getY()

         minX = math.min(minX, x)
         maxX = math.max(maxX, x)
         minY = math.min(minY, y)
         maxY = math.max(maxY, y)
      end

      width = maxX - minX
      height = maxY - minY
      factorX = (width + 24.0) / width
      factorY = (height + 24.0) / height

      -- print(string.format("width: %f, height: %f, factorX: %f, factorY: %f", width, height, factorX, factorY))

      for key, value in pairs(path) do
         value:setX(minX + (value:getX() - minX) * factorX - 12)
         value:setY(minY + (value:getY() - minY) * factorY - 12)
      end

      -- generate patrol path
      length = 0.0
      posPrev = path[1]

      -- calc overall loop length
      i = 1
      for key, value in pairs(path) do
         l = (value - posPrev):getLength()
         length = length + l
         posPrev = value
      end

      value = path[1]
      length = length + (value - posPrev):getLength()

      -- create loop and leave space for one extra item at the end
      posPrev = path[1]
      i = 1
      l = 0.0
      for key, value in pairs(path) do

         l = l + (value - posPrev):getLength()
         mPatrolPath[i] = Key:create{x = value:getX(), y = value:getY(), time = l / length}
         i = i + 1
         posPrev = value
      end

      value = path[1]
      mPatrolPath[i]  = Key:create{x = value:getX(), y = value:getY(), time = 1.0}
   end

   -- the length of 3 tiles next to each other is the default length for the speed computation
   mSpeed = 192 / length

   -- print(string.format("length: %f, speed: %f", length, mSpeed))

   -- debug the loop
   -- for key, value in pairs(mPatrolPath) do
   --    print(string.format("path: %d: %f, %f, time: %f", key, value.x, value.y, value.time))
   -- end
end


------------------------------------------------------------------------------------------------------------------------
function retrieveProperties()
   updateProperties(properties)
end


------------------------------------------------------------------------------------------------------------------------
function movedTo(x, y)
end


------------------------------------------------------------------------------------------------------------------------
function writeProperty(key, value)
   if (key == "inverse_path") then
      mPointsLeft = not (value == "1")
   end
end

