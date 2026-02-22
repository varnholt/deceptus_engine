require "data/scripts/enemies/constants"
v2d = require "data/scripts/enemies/vectorial2"


------------------------------------------------------------------------------------------------------------------------
properties = {
   static_body = true,
   sprite = "data/sprites/enemy_bonefish.png",
   damage = 4
}


------------------------------------------------------------------------------------------------------------------------
mPosition = v2d.Vector2D(0, 0)
mSpeed = 1.0
mFrequency = 1.0
mAmplitude = 1.0
mPlayerPosition = v2d.Vector2D(0, 0)
mCenter = v2d.Vector2D(0, 0)
mDistance = 0
mElapsed = math.random(0, 3)
mSpriteIndex = 0
mDirectionX = 1.0  -- X component of movement direction (normalized)
mDirectionY = 0.0  -- Y component of movement direction (normalized)
mPrevOffset = 0.0  -- Previous sine offset for sprite flip detection


------------------------------------------------------------------------------------------------------------------------
function initialize()

   patrolPath = {}
   patrolIndex = 1
   patrolEpsilon = 1.0

   addShapeRect(0.3, 0.15, 0, 0.05)
   updateSpriteRect(0, 0, 0, 72, 48)
end


------------------------------------------------------------------------------------------------------------------------
function update(dt)
   yOffset = 0
   updateSprite = false

   -- get sprite index
   mElapsed = mElapsed + dt
   spriteIndex = math.floor(math.fmod(mElapsed * 20.0, 30))

   -- Calculate position using direction factors (no branching needed)
   local offset = 0.5 * math.sin(mElapsed * mSpeed) * mDistance
   local posX = mCenter:getX() + offset * mDirectionX
   local posY = mCenter:getY() + offset * mDirectionY

   -- Add perpendicular oscillation (direction rotated 90 degrees)
   local perpX = -mDirectionY
   local perpY = mDirectionX
   local perpOffset = math.sin(mElapsed * mFrequency) * mAmplitude
   posX = posX + perpX * perpOffset
   posY = posY + perpY * perpOffset

   -- Detect movement direction for sprite flip (only when moving primarily horizontally)
   if math.abs(mDirectionX) > math.abs(mDirectionY) then
      if offset > mPrevOffset then
         mPointsLeft = false  -- Moving right
      else
         mPointsLeft = true   -- Moving left
      end
   end
   mPrevOffset = offset

   -- update transform
   setTransform(posX, posY, 0.0)

   -- update sprite index
   if (spriteIndex ~= mSpriteIndex) then
      mSpriteIndex = spriteIndex
      updateSprite = true
   end

   -- Handle sprite flipping based on movement direction
   if math.abs(mDirectionX) > math.abs(mDirectionY) and mPointsLeft then
      yOffset = 0   -- Normal when moving left
   else
      yOffset = 48  -- Flip when moving right (or vertical movement)
   end

   if (updateSprite) then
      updateSpriteRect(0, mSpriteIndex * 72, yOffset, 72, 48) -- x, y, width, height
   end

end


------------------------------------------------------------------------------------------------------------------------
function writeProperty(parameter, value)

   if (parameter == "speed") then
      mSpeed = value
   elseif (parameter == "frequency") then
      mFrequency = value
   elseif (parameter == "amplitude") then
      mAmplitude = value
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

   if (name == "path") then
      patrolPath = v

      -- compute movement direction as normalized factors
      local leftArr = patrolPath[0]
      local left = v2d.Vector2D(leftArr:getX(), leftArr:getY())
      local rightArr = patrolPath[1]
      local right = v2d.Vector2D(rightArr:getX(), rightArr:getY())
      local dx = right:getX() - left:getX()
      local dy = right:getY() - left:getY()

      -- calculate center point
      mCenter = v2d.Vector2D((left:getX() + right:getX()) / 2.0, (left:getY() + right:getY()) / 2.0)

      -- calculate distance and normalized direction
      mDistance = math.sqrt(dx * dx + dy * dy)
      if mDistance > 0 then
         mDirectionX = dx / mDistance
         mDirectionY = dy / mDistance
      end
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
