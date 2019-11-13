------------------------------------------------------------------------------------------------------------------------
require "data/scripts/enemies/constants"
v2d = require "data/scripts/enemies/vectorial2"


------------------------------------------------------------------------------------------------------------------------
properties = {
   staticBody = true,
   sprite = "data/sprites/enemy_watermine.png",
   damage = 200
}


------------------------------------------------------------------------------------------------------------------------
mPosition = v2d.Vector2D(0, 0)
mPlayerPosition = v2d.Vector2D(0, 0)
mWidth = 0
mElapsed = math.random(0, 3)
mSpriteIndex = 0
mPrevX = 0.0


------------------------------------------------------------------------------------------------------------------------
function initialize()
   addShapeRect(0.2, 0.2, -0.05, 0.0)
   updateSpriteRect(0, 0, 48, 24)
end


------------------------------------------------------------------------------------------------------------------------
function update(dt)
   yOffset = 0
   updateSprite = false

   -- get sprite index
   mElapsed = mElapsed + dt
   spriteIndex = math.floor(math.fmod(mElapsed * 10.0, 4))

   -- get sprite direction
   y = 0.5 * math.sin(mElapsed) * mWidth

   -- update transform
   -- setTransform(mPosition:getX(), mPosition:getY() + y, 0.0)

   -- update sprite index
   if (index ~= mSpriteIndex) then
      mSpriteIndex = spriteIndex
      updateSprite = true
   end

   if (updateSprite) then
      updateSpriteRect(mSpriteIndex * 48, yOffset, 48, 24) -- x, y, width, height
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
function movedTo(x, y)
   mPosition = v2d.Vector2D(x, y)

   print(string.format("moved to: %f, %f", x, y))
end


------------------------------------------------------------------------------------------------------------------------
function playerMovedTo(x, y)
   mPlayerPosition = v2d.Vector2D(x, y)
end


------------------------------------------------------------------------------------------------------------------------
function timeout(id)
end
