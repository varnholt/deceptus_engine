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
mDistance = 0  -- Will hold either horizontal or vertical distance depending on path orientation
mElapsed = math.random(0, 3)
mSpriteIndex = 0
mPointsLeft = false  -- For horizontal movement direction (true = moving left, false = moving right)
mPrevCoord = 0.0     -- Previous coordinate (x for horizontal, y for vertical)
mIsVerticalPath = false  -- Flag to determine if path is vertical or horizontal


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

   -- Calculate position based on path orientation
   local offset = 0.5 * math.sin(mElapsed * mSpeed) * mDistance

   -- Determine position based on path orientation
   local posX, posY

   if mIsVerticalPath then
      -- Vertical movement along the path
      posY = mCenter:getY() + offset
      posX = mCenter:getX()
   else
      -- Horizontal movement along the path
      posX = mCenter:getX() + offset
      posY = mCenter:getY()

      -- Check direction for sprite flipping (horizontal movement only)
      if (offset > mPrevCoord) then
         mPointsLeft = false  -- Moving right
         yOffset = 48  -- Flip sprite horizontally when moving right
      else
         mPointsLeft = true   -- Moving left
      end
   end

   mPrevCoord = offset

   -- Apply perpendicular oscillation based on path orientation
   if mIsVerticalPath then
      -- For vertical paths, add horizontal oscillation
      posX = posX + math.sin(mElapsed * mFrequency) * mAmplitude
   else
      -- For horizontal paths, add vertical oscillation
      posY = posY + math.sin(mElapsed * mFrequency) * mAmplitude
   end

   -- update transform
   setTransform(posX, posY, 0.0)

   -- update sprite index
   if (spriteIndex ~= mSpriteIndex) then
      mSpriteIndex = spriteIndex
      updateSprite = true
   end

   -- Handle sprite flipping based on movement direction
   -- For horizontal paths, apply left/right flip based on movement direction
   -- For vertical paths, we don't apply horizontal flip since the fish is moving up/down
   if not mIsVerticalPath then
       -- Only apply horizontal flip for horizontal movement
       if mPointsLeft then
           yOffset = 0   -- Normal when moving left
       else
           yOffset = 48  -- Flip when moving right
       end
   else
       -- For vertical paths, we keep yOffset as 0 (no horizontal flip)
       -- The sprite remains in its default orientation during vertical movement
       yOffset = 0
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

      -- Calculate the angle between the two path points to determine orientation
      local leftArr = patrolPath[0]
      local left = v2d.Vector2D(leftArr:getX(), leftArr:getY())
      local rightArr = patrolPath[1]
      local right = v2d.Vector2D(rightArr:getX(), rightArr:getY())

      -- Calculate angle in degrees
      local dx = right:getX() - left:getX()
      local dy = right:getY() - left:getY()
      local angleRad = math.atan2(dy, dx)
      local angleDeg = math.deg(angleRad)
      
      -- Normalize angle to -180 to 180 range
      if angleDeg > 180 then
          angleDeg = angleDeg - 360
      elseif angleDeg < -180 then
          angleDeg = angleDeg + 360
      end
      
      -- Determine if path is vertical or horizontal
      -- -45 to 45 degrees means horizontal movement
      -- Otherwise it's vertical (or diagonal treated as vertical)
      mIsVerticalPath = not (angleDeg >= -45 and angleDeg <= 45)
      
      -- Calculate center point
      mCenter = v2d.Vector2D((left:getX() + right:getX()) / 2.0, (left:getY() + right:getY()) / 2.0)
      
      -- Calculate distance to use for movement (either horizontal or vertical)
      if mIsVerticalPath then
          mDistance = math.sqrt(dx * dx + dy * dy)  -- Use full distance for vertical paths
      else
          mDistance = math.abs(dx)  -- Use horizontal distance for horizontal paths
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