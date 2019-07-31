
require "data/scripts/enemies/constants"
v2d = require "data/scripts/enemies/vectorial2"

------------------------------------------------------------------------------------------------------------------------
properties = {
   staticBody = true,
   sprite = "data/sprites/klonk.png",
   damage = 200
}


------------------------------------------------------------------------------------------------------------------------
mPosition = v2d.Vector2D(0, 0)
mPlayerPosition = v2d.Vector2D(0, 0)
mSpriteIndex = 0
mElapsed = 0.0
mCycle = 0

------------------------------------------------------------------------------------------------------------------------
function initialize()

   addShapeRect(0.2, 0.2, 0.0, 0.0)
   updateSpriteRect(0, 2 * 72, 72, 72) -- x, y, width, height
end


------------------------------------------------------------------------------------------------------------------------
function timeout(id)
   -- print(string.format("timeout: %d", id))
end


------------------------------------------------------------------------------------------------------------------------
function retrieveProperties()
   updateProperties(properties)
end


------------------------------------------------------------------------------------------------------------------------
function movedTo(x, y)
   -- print(string.format("moved to: %f, %f", x, y))
   mPosition = v2d.Vector2D(x, y)
end


------------------------------------------------------------------------------------------------------------------------
function playerMovedTo(x, y)
   -- print(string.format("player moved to: %f, %f", x, y))
   mPlayerPosition = v2d.Vector2D(x, y)
end


------------------------------------------------------------------------------------------------------------------------
function update(dt)

   -- make sure block is on same x as player
   if (mCycle == 0) then
      if (mPosition:getX() // 24 == mPlayerPosition:getX() // 24) then

         -- make sure stone is not too far away (10 tiles) and above player
         yDiff = mPosition:getY() // 24 - mPlayerPosition:getY() // 24

         if (yDiff < 0 and yDiff > -10) then

            -- make sure there's nothing in the way
            if (
               isPhsyicsPathClear(
                  mPosition:getX(),
                  mPosition:getY(),
                  mPlayerPosition:getX(),
                  mPlayerPosition:getY()
               )
            )
            then
               -- activate the stone
               mCycle = 1
            end
         end
      end
   end

   -- stone starts falling
   if (mCycle == 1) then

      mElapsed = mElapsed + dt
      mSpriteIndex = math.floor(mElapsed * 10.0)

      -- maybe close eyes again if player is out of sight?
      if (mSpriteIndex < 12) then
         updateSpriteRect(mSpriteIndex * 72, 2 * 72, 72, 72)
      else
         makeDynamic()
         mCycle = 2
      end

   end

   -- block starts falling
   if (mCycle == 2) then
      velocity = getLinearVelocity()

      if (velocity[2] > 1.0) then
         mCycle = 3
      end
   end

   -- stone hit the floor
   if (mCycle == 3) then
      velocity = getLinearVelocity()

      if (velocity[2] <= 0.01) then
         velocity = getLinearVelocity()
         mCycle = 4
         setActive(false)
      end

      -- todo: player hit ground animation
   end

end


------------------------------------------------------------------------------------------------------------------------
function setPath(name, table)
end

