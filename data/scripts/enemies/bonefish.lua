------------------------------------------------------------------------------------------------------------------------
require "data/scripts/enemies/constants"
v2d = require "data/scripts/enemies/vectorial2"


------------------------------------------------------------------------------------------------------------------------
properties = {
   staticBody = true,
   sprite = "data/sprites/enemy_bonefish.png",
   damage = 200,
   tick = 0
}


------------------------------------------------------------------------------------------------------------------------
mPosition = v2d.Vector2D(0, 0)
mPlayerPosition = v2d.Vector2D(0, 0)
mCenter = v2d.Vector2D(0, 0)
mWidth = 0
mElapsed = 0.0
mSpriteIndex = 0
mPointsLeft = false
mPrevX = 0.0


------------------------------------------------------------------------------------------------------------------------
function initialize()

   patrolPath = {}
   patrolIndex = 1
   patrolEpsilon = 1.0

   addShapeRect(0.2, 0.2, -0.05, 0.0)
   updateSpriteRect(0, 0, 48, 24)
end


------------------------------------------------------------------------------------------------------------------------
function update(dt)

   -- need a way to pass back properties to script
   -- print(tick)

   yOffset = 0
   updateSprite = false

   -- get sprite index
   mElapsed = mElapsed + dt
   spriteIndex = math.floor(math.fmod(mElapsed * 10.0, 8))

   -- get sprite direction
   x = 0.5 * math.sin(mElapsed) * mWidth

   if (x > mPrevX) then
      yOffset = 24
   end

   mPrevX = x

   -- update transform
   setTransform(mCenter:getX() + x, mCenter:getY(), 0.0)

   -- update sprite index
   if (index ~= mSpriteIndex) then
      mSpriteIndex = spriteIndex
      updateSprite = true
   end

   if (pointsLeft ~= mPointsLeft) then
      mPointsLeft = pointsLeft
      updateSprite = true
   end

   if (updateSprite) then
      updateSpriteRect(mSpriteIndex * 48, yOffset, 48, 24) -- x, y, width, height
   end

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
         -- print(string.format("v%d: %f, %f", (i - 1) / 2, x, y))
         v[(i - 1) / 2] = v2d.Vector2D(x, y)
      end

      i = i + 1
   end

   if (name == "patrol_path") then
      patrolPath = v

      -- just store the center and the width of that path
      local leftArr = patrolPath[0]
      local left = v2d.Vector2D(leftArr:getX(), leftArr:getY())
      local rightArr = patrolPath[1]
      local right = v2d.Vector2D(rightArr:getX(), rightArr:getY())

      mCenter = v2d.Vector2D((left:getX() + right:getX()) / 2.0, (left:getY() + right:getY()) / 2.0)
      mWidth = right:getX() - left:getX()
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


------------------------------------------------------------------------------------------------------------------------
function playerMovedTo(x, y)
   mPlayerPosition = v2d.Vector2D(x, y)
end


------------------------------------------------------------------------------------------------------------------------
function timeout(id)
end
