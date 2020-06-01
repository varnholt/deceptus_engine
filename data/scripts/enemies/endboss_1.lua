require "data/scripts/enemies/constants"
require "data/scripts/enemies/helpers"
v2d = require "data/scripts/enemies/vectorial2"

-- enemy configuration
properties = {
   sprite = "data/sprites/enemy_endboss_1.png",
   acceleration_ground = 0.1
}

------------------------------------------------------------------------------------------------------------------------
mKeyPressed = 0
mPosition = v2d.Vector2D(0, 0)
mPlayerPosition = v2d.Vector2D(0, 0)
mFireDistance = 300


------------------------------------------------------------------------------------------------------------------------
function initialize()
   wait = false

   addShapeRect(0.6, 1.05, 0.25, 0.0)
   updateSpriteRect(0, 0, 80, 104)

   addWeapon(500, 80, 0,0, 0,0.01666, 1.0,0.01666, 1.0,0, 0,0) -- interval, damage, radius/shape
   updateBulletTexture(0, "data/sprites/enemy_endboss_1.png", 0, 112, 49, 8) -- index, path, x, y, width, height
end


------------------------------------------------------------------------------------------------------------------------
function timeout(id)
end


------------------------------------------------------------------------------------------------------------------------
function retrieveProperties()
   updateProperties(properties)
end


------------------------------------------------------------------------------------------------------------------------
function keyPressed(key)
   mKeyPressed = (mKeyPressed | key)
end


------------------------------------------------------------------------------------------------------------------------
function keyReleased(key)
   mKeyPressed = mKeyPressed & (~key)
end


------------------------------------------------------------------------------------------------------------------------
function setPatrolPositions(x1, y1, x2, y2)
end


------------------------------------------------------------------------------------------------------------------------
function movedTo(x, y)
   -- print(string.format("moved to: %f, %f", x, y))
   mPosition = v2d.Vector2D(x, y)
end


function playerMovedTo(x, y)
   -- print(string.format("player moved to: %f, %f", x, y))
   mPlayerPosition = v2d.Vector2D(x, y)
end


------------------------------------------------------------------------------------------------------------------------
function updateShootCondition()

   fireWeapon(
      0,
      mPosition:getX() - 70,
      mPosition:getY() - 1,
      -0.04,
      0.0
   );

end


------------------------------------------------------------------------------------------------------------------------
function update(dt)
   updateShootCondition()
   updateKeysPressed(mKeyPressed)
end




