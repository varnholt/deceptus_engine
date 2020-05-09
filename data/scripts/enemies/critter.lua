------------------------------------------------------------------------------------------------------------------------
require "data/scripts/enemies/constants"
v2d = require "data/scripts/enemies/vectorial2"


------------------------------------------------------------------------------------------------------------------------
properties = {
   staticBody = true,
   sprite = "data/sprites/enemy_critter.png",
   damage = 200
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
mElapsed = math.random(0, 3)
mSpriteIndex = 0
mPointsLeft = false
mLoop = false


------------------------------------------------------------------------------------------------------------------------
function initialize()

   patrolPath = {}

   addShapeRect(0.2, 0.2, -0.05, 0.0)
   updateSpriteRect(0, 0, 48, 24)
end


------------------------------------------------------------------------------------------------------------------------
function update(dt)
end


------------------------------------------------------------------------------------------------------------------------
function setPath(name, table)
   -- print(string.format("Received %d arguments:", #table))

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
      patrolPath = v
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

