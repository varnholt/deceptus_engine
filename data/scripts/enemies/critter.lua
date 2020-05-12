
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
   damage = 20
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
mCenter = v2d.Vector2D(0, 0)
mElapsed = 0.0
mSpriteIndex = 0
mPointsLeft = false
mLoop = false


------------------------------------------------------------------------------------------------------------------------
function initialize()

   mPatrolPath = {}

   addShapeRect(0.2, 0.2, -0.05, 0.0)
   updateSpriteRect(0, 0, 48, 48)
end


------------------------------------------------------------------------------------------------------------------------
-- 0: right, horizontal, up
-- 1: left, horizontal, up
-- 2: right, horizontal, down
-- 3: left, horizontal, down
-- 4: right, vertical, down
-- 5: right, vertical, up
-- 6: left, vertical, down
-- 7: left, vertical, up
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

   epsilon = 0.001

   if (dx > epsilon) then
      row = 2
   elseif  (dx < -epsilon) then
      row = 1
   end

   if  (dy > epsilon) then
      row = 6
   elseif (dy < -epsilon) then
      row = 5
   end

   updateSpriteRect(0, row * 48, 48, 48)
end


------------------------------------------------------------------------------------------------------------------------
function update(dt)

   if (#mPatrolPath == 0) then
      return
   end

   mElapsed = mElapsed + dt

   p = getValueLinear(mPatrolPath, math.fmod(mElapsed * 0.25, 1.0));

   setTransform(
      p:getX(),
      p:getY(),
      0.0
   )

   updateSprite(p)

   mPosition = p

end


------------------------------------------------------------------------------------------------------------------------
function setPath(name, table)
   print(string.format("Received %s, %d arguments:", name, #table))

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

   if (name == "patrol_path") then

      -- create loop and leave space for one extra item at the end
      i = 1
      for key, value in pairs(path) do
         mPatrolPath[i] = Key:create{x = value:getX(), y = value:getY(), time = (i - 1) / (#path + 1)}
         i = i + 1
      end

      -- close the loop
      value = path[1]
      mPatrolPath[i]  = Key:create{x = value:getX(), y = value:getY(), time = 1.0}
   end

   -- debug the loop
   for key, value in pairs(mPatrolPath) do
      print(string.format("patrolpath: %d: %f, %f, time: %f", key, value.x, value.y, value.time))
   end
end


------------------------------------------------------------------------------------------------------------------------
function retrieveProperties()
   updateProperties(properties)
end


------------------------------------------------------------------------------------------------------------------------
function movedTo(x, y)
   mPosition = v2d.Vector2D(x, y)
end

