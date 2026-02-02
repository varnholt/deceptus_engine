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
   static_body = false,
   sprite = "data/sprites/orb.png",
   damage = 2,
   speed = 1.0,
   anim_speed = 10.0
}

------------------------------------------------------------------------------------------------------------------------
mPosition = v2d.Vector2D(0, 0)

-- spawn / alignment
mSpawnX, mSpawnY = nil, nil
mHasSpawn = false
mNeedsAlign = true

mElapsed = 0.0

-- movement
mSpeed = 1.0
mSpeedMultiplier = 1.0
mPathLength = 192.0

-- animation
mAnimFrame = 0
mAnimSpeed = 10.0

EPSILON = 0.0001

------------------------------------------------------------------------------------------------------------------------
function initialize()
   mPatrolPath = {}
   addShapeCircle(0.23, 0.0, 0.0)
   updateSpriteRect(0, 0, 0, 24, 24)
end

------------------------------------------------------------------------------------------------------------------------
-- Align orb to closest point on the path based on editor placement
function alignToSpawnIfNeeded()
   if (not mNeedsAlign) then return end
   if (not mHasSpawn) then return end
   if (mPatrolPath == nil or #mPatrolPath == 0) then return end

   local closestTime = 0.0
   local shortestDistanceSquared = 1e30

   for i = 1, #mPatrolPath do
      local k = mPatrolPath[i]
      local dx = (k.x - mSpawnX)
      local dy = (k.y - mSpawnY)
      local distanceSquared = dx*dx + dy*dy
      if (distanceSquared < shortestDistanceSquared) then
         shortestDistanceSquared = distanceSquared
         closestTime = k.time
      end
   end

   local speedDenominator = (mSpeed * 0.2)
   if (speedDenominator > EPSILON) then
      mElapsed = closestTime / speedDenominator
   else
      mElapsed = 0.0
   end

   mNeedsAlign = false
end

------------------------------------------------------------------------------------------------------------------------
function update(dt)
   if (mPatrolPath == nil or #mPatrolPath == 0) then
      return
   end

   -- normalize dt for movement too (some engines send ms)
   local dt_move = dt
   if (dt_move > 1.0) then dt_move = dt_move / 1000.0 end

   alignToSpawnIfNeeded()

   mElapsed = mElapsed + dt_move

   local val = math.fmod(mSpeed * mElapsed * 0.2, 1.0)
   local p = getValueLinear(mPatrolPath, val)
   setTransform(p:getX(), p:getY(), 0.0)

   -- normalize dt for animation
   local dt_sec = dt_move

   mAnimFrame = mAnimFrame + dt_sec * mAnimSpeed
   local frame = math.floor(mAnimFrame) % 8
   updateSpriteRect(0, frame * 120, 0, 120, 120)

   mPosition = p
end

------------------------------------------------------------------------------------------------------------------------
-- Sets the path for the orb to follow
-- @param name string - must be "path" to be processed
-- @param data table - can be one of the following data structures:
--    1. Array-like table with alternating x,y coordinates: {x1, y1, x2, y2, ...}
--    2. Dictionary-like table with numeric indices: {[1] = x1, [2] = y1, [3] = x2, [4] = y2, ...}
function setPath(name, data)
   if (name ~= "path") then return end
   if (data == nil) then return end

   local path = {}

   if (#data ~= nil and #data >= 2) then
      local idx = 1
      for i = 1, #data, 2 do
         local x = data[i]
         local y = data[i + 1]
         if (x ~= nil and y ~= nil) then
            path[idx] = v2d.Vector2D(x, y)
            idx = idx + 1
         end
      end
   else
      local keys = {}
      for k, _ in pairs(data) do
         local nk = tonumber(k)
         if (nk ~= nil) then
            keys[#keys + 1] = nk
         end
      end
      _G.table.sort(keys)

      local idx = 1
      for i = 1, #keys, 2 do
         local x = data[keys[i]]
         local y = data[keys[i + 1]]
         if (x ~= nil and y ~= nil) then
            path[idx] = v2d.Vector2D(x, y)
            idx = idx + 1
         end
      end
   end

   if (#path == 0) then return end

   -- expand path by sprite size
   local minX, maxX =  9999, -9999
   local minY, maxY =  9999, -9999

   for i = 1, #path do
      local x = path[i]:getX()
      local y = path[i]:getY()
      minX = math.min(minX, x)
      maxX = math.max(maxX, x)
      minY = math.min(minY, y)
      maxY = math.max(maxY, y)
   end

   local width  = maxX - minX
   local height = maxY - minY

   local factorX = (math.abs(width)  > EPSILON) and ((width  + 24.0) / width)  or 1.0
   local factorY = (math.abs(height) > EPSILON) and ((height + 24.0) / height) or 1.0

   for i = 1, #path do
      local v = path[i]
      v:setX(minX + (v:getX() - minX) * factorX - 12)
      v:setY(minY + (v:getY() - minY) * factorY - 12)
   end

   -- build patrol keys
   mPatrolPath = {}

   local length = 0.0
   local prev = path[1]

   for i = 2, #path do
      local v = path[i]
      length = length + (v - prev):getLength()
      prev = v
   end
   length = length + (path[1] - prev):getLength()

   if (length <= EPSILON) then
      return
   end

   local acc = 0.0
   local out = 1

   -- first key at time 0 on first point
   mPatrolPath[out] = Key:create{x = path[1]:getX(), y = path[1]:getY(), time = 0.0}
   out = out + 1

   prev = path[1]
   for i = 2, #path do
      local v = path[i]
      acc = acc + (v - prev):getLength()
      mPatrolPath[out] = Key:create{x = v:getX(), y = v:getY(), time = acc / length}
      out = out + 1
      prev = v
   end

   -- close loop
   mPatrolPath[out] = Key:create{x = path[1]:getX(), y = path[1]:getY(), time = 1.0}

   mPathLength = length
   mSpeed = (192 / mPathLength) * mSpeedMultiplier

   mNeedsAlign = true
end

------------------------------------------------------------------------------------------------------------------------
function retrieveProperties()
   updateProperties(properties)
end

------------------------------------------------------------------------------------------------------------------------
function movedTo(x, y)
   if (not mHasSpawn) then
      mSpawnX, mSpawnY = x, y
      mHasSpawn = true
      mNeedsAlign = true
   end
end

------------------------------------------------------------------------------------------------------------------------
function writeProperty(key, value)
   if (key == "speed") then
      mSpeedMultiplier = tonumber(value) or 1.0
      if (mPathLength ~= nil and mPathLength > 0.0) then
         mSpeed = (192 / mPathLength) * mSpeedMultiplier
      end
   elseif (key == "animation_speed") then
      local v = tonumber(value)
      if (v ~= nil and v > 0) then
         mAnimSpeed = v
      end
   end
end
