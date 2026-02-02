------------------------------------------------------------------------------------------------------------------------
require "data/scripts/enemies/constants"
v2d = require "data/scripts/enemies/vectorial2"

------------------------------------------------------------------------------------------------------------------------
-- Simple patrol bat (Bonefish-style)
-- Moves back and forth between two points provided by setPath("path", {...})
-- Optional wobble and configurable direction ("horizontal" or "vertical")
------------------------------------------------------------------------------------------------------------------------

properties = {
   static_body = true,
   sprite = "data/sprites/enemy_bat_2.png",
   damage = 1
}

------------------------------------------------------------------------------------------------------------------------
-- patrol config
mCenter = v2d.Vector2D(0, 0)
mWidth  = 0.0
mHeight = 0.0

mSpeed = 1.0           -- patrol speed (higher = faster)
mFrequency = 2.0       -- wobble frequency
mAmplitude = 6.0       -- wobble amplitude (px)
mDirection = "horizontal"

mElapsed = math.random(0, 3)

-- sprite / animation
SPRITE_W = 72
SPRITE_H = 72
SPRITE_FRAMES = 9
ANIM_SPEED = 20.0

mSpriteIndex = 0
mPointsLeft = false
mPrevX = 0.0
mPrevY = 0.0

------------------------------------------------------------------------------------------------------------------------
function initialize()
   addHitbox(-18, -12, 36, 24)
   addShapeCircle(0.3, 0.0, 0.0)

   updateSpriteRect(0, 0, 0, SPRITE_W, SPRITE_H)
   setZ(30)
end

------------------------------------------------------------------------------------------------------------------------
function update(dt)
   mElapsed = mElapsed + dt

   -- animation frame
   local spriteIndex = math.floor(math.fmod(mElapsed * ANIM_SPEED, SPRITE_FRAMES))

   -- base back-and-forth motion
   local x = 0.0
   local y = 0.0
   local yOffset = 0
   local updateSprite = false
   local pointsLeft = mPointsLeft

   if (mDirection == "vertical") then
      x = math.sin(mElapsed * mFrequency) * mAmplitude
      y = 0.5 * math.sin(mElapsed * mSpeed) * mHeight
      mPrevY = y
   else
      x = 0.5 * math.sin(mElapsed * mSpeed) * mWidth
      y = math.sin(mElapsed * mFrequency) * mAmplitude
      mPrevX = x
   
      -- Facing based on movement direction
      if (x > mPrevX) then
         pointsLeft = false
         yOffset = 0
      else
         pointsLeft = true
         yOffset = SPRITE_H
      end      
   end

   -- apply transform
   setTransform(mCenter:getX() + x, mCenter:getY() + y, 0.0)

   -- sprite updates
   if (spriteIndex ~= mSpriteIndex) then
      mSpriteIndex = spriteIndex
      updateSprite = true
   end

   if (mDirection ~= "vertical") then
      if (pointsLeft ~= mPointsLeft) then
         mPointsLeft = pointsLeft
         updateSprite = true
      end
   end

   if (updateSprite) then
      updateSpriteRect(0, mSpriteIndex * SPRITE_W, yOffset, SPRITE_W, SPRITE_H)
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
   elseif (parameter == "direction") then
      -- expects "horizontal" or "vertical"
      mDirection = value
   end
end

------------------------------------------------------------------------------------------------------------------------
function setPath(name, table)
   -- Parses table as {x1, y1, x2, y2, ...} (same as bonefish.lua)
   local i = 0
   local x = 0.0
   local y = 0.0
   local v = {}

   for key, value in pairs(table) do
      if ((i % 2) == 0) then
         x = value
      else
         y = value
         v[(i - 1) / 2] = v2d.Vector2D(x, y)
      end
      i = i + 1
   end

   if (name == "path") then
      local aArr = v[0]
      local bArr = v[1]
      if (aArr == nil or bArr == nil) then
         -- Need at least 2 points; if missing, keep current center/ranges
         return
      end

      local a = v2d.Vector2D(aArr:getX(), aArr:getY())
      local b = v2d.Vector2D(bArr:getX(), bArr:getY())

      -- center is midpoint, ranges come from delta between the points
      mCenter = v2d.Vector2D((a:getX() + b:getX()) / 2.0, (a:getY() + b:getY()) / 2.0)
      mWidth  = (b:getX() - a:getX())
      mHeight = (b:getY() - a:getY())
   end
end

------------------------------------------------------------------------------------------------------------------------
function retrieveProperties()
   updateProperties(properties)
end

------------------------------------------------------------------------------------------------------------------------
function movedTo(x, y)
   -- If no path has been set, let the placed position act as the center.
   if (mCenter:getX() == 0 and mCenter:getY() == 0) then
      mCenter = v2d.Vector2D(x, y)
   end
end

------------------------------------------------------------------------------------------------------------------------
function playerMovedTo(x, y)
   -- Not used for patrol
end

------------------------------------------------------------------------------------------------------------------------
function hit(dmg)
   -- Simple patrol bat doesn't have health logic here
end

------------------------------------------------------------------------------------------------------------------------
function timeout(id)
end
