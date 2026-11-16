properties = {
   static_body = true,
   sensor = true,
   sprite = "data/sprites/insect.png",
   damage = 0
}

FRAME_SIZE_PX = 24
HALF_FRAME_PX = FRAME_SIZE_PX / 2
DIAGONAL = 1 / math.sqrt(2)
-- rows in the artwork: up, down, right, left, up/right, up/left, down/right, down/left.
DIRECTIONS = {
   {0, -1}, {0, 1}, {1, 0}, {-1, 0},
   {DIAGONAL, -DIAGONAL}, {-DIAGONAL, -DIAGONAL},
   {DIAGONAL, DIAGONAL}, {-DIAGONAL, DIAGONAL}
}

mConfig = {
   speed = 12, -- pixels per second
   walk_duration_min_ms = 1000,
   walk_duration_max_ms = 2000,
   rest_duration_min_ms = 3000,
   rest_duration_max_ms = 4000,
   region_width_px = 96,
   region_height_px = 96
}

mRegionX, mRegionY = 0, 0
mMinX, mMaxX, mMinY, mMaxY = 0, 0, 0, 0
mX, mY = 0, 0
mDestinationX, mDestinationY = 0, 0
mDirection = 1
mRemaining = 0
mAnimationElapsed = 0
mResting = false
mStarted = false
mHasSpawn = false

function initialize()
   addShapeCircle(0.1, 0, 0)
   setSpriteOrigin(0, HALF_FRAME_PX, HALF_FRAME_PX)
   updateSpriteRect(0, 0, 0, FRAME_SIZE_PX, FRAME_SIZE_PX)
end

function retrieveProperties()
   updateProperties(properties)
end

function movedTo(x, y)
   -- movedTo is called every frame; only the initial position defines the region.
   if (not mHasSpawn) then
      mRegionX, mRegionY = x, y
      mHasSpawn = true
   end
end

function randomDuration(minimum, maximum)
   local low = math.min(minimum, maximum)
   local high = math.max(minimum, maximum)
   return (low + math.random() * (high - low)) / 1000
end

function startResting()
   mResting = true
   mRemaining = randomDuration(mConfig.rest_duration_min_ms, mConfig.rest_duration_max_ms)
   mAnimationElapsed = 0
end

function startWalking()
   local candidates = {}
   for index, direction in ipairs(DIRECTIONS) do
      local dx, dy = direction[1], direction[2]
      local blocked = (dx < 0 and mX <= mMinX) or (dx > 0 and mX >= mMaxX)
         or (dy < 0 and mY <= mMinY) or (dy > 0 and mY >= mMaxY)
      if (not blocked) then
         candidates[#candidates + 1] = index
      end
   end

   -- a rectangle too small to move in still supports the idle animation.
   if (#candidates == 0) then
      startResting()
      return
   end

   mDirection = candidates[math.random(#candidates)]
   local direction = DIRECTIONS[mDirection]
   local vx, vy = direction[1] * mConfig.speed, direction[2] * mConfig.speed
   local duration = randomDuration(mConfig.walk_duration_min_ms, mConfig.walk_duration_max_ms)
   if (vx > 0) then
      duration = math.min(duration, (mMaxX - mX) / vx)
   elseif (vx < 0) then
      duration = math.min(duration, (mMinX - mX) / vx)
   end
   if (vy > 0) then
      duration = math.min(duration, (mMaxY - mY) / vy)
   elseif (vy < 0) then
      duration = math.min(duration, (mMinY - mY) / vy)
   end

   mDestinationX = math.max(mMinX, math.min(mMaxX, mX + vx * duration))
   mDestinationY = math.max(mMinY, math.min(mMaxY, mY + vy * duration))
   mRemaining = duration
   mAnimationElapsed = 0
   mResting = false
end

function startRoaming()
   -- inset by half a frame so the entire sprite stays inside the drawn rectangle.
   local width, height = mConfig.region_width_px, mConfig.region_height_px
   local paddingX = math.min(HALF_FRAME_PX, width / 2)
   local paddingY = math.min(HALF_FRAME_PX, height / 2)
   mMinX, mMaxX = mRegionX + paddingX, mRegionX + width - paddingX
   mMinY, mMaxY = mRegionY + paddingY, mRegionY + height - paddingY
   mX, mY = mRegionX + width / 2, mRegionY + height / 2
   mStarted = true
   startWalking()
end

function update(dt)
   if (not mHasSpawn) then
      return
   end
   -- properties arrive after initialize; defer the first walk until they are all available.
   if (not mStarted) then
      startRoaming()
   end

   local remaining = dt -- the engine passes seconds
   while (remaining > 0) do
      local elapsed = math.min(remaining, mRemaining)
      remaining = remaining - elapsed
      mRemaining = mRemaining - elapsed
      mAnimationElapsed = mAnimationElapsed + elapsed
      if (not mResting) then
         local direction = DIRECTIONS[mDirection]
         mX = mDestinationX - direction[1] * mConfig.speed * mRemaining
         mY = mDestinationY - direction[2] * mConfig.speed * mRemaining
         mX = math.max(mMinX, math.min(mMaxX, mX))
         mY = math.max(mMinY, math.min(mMaxY, mY))
      end

      if (mRemaining <= 0) then
         if (mResting) then
            startWalking()
         else
            startResting()
         end
      end
   end

   setTransform(mX, mY, 0)
   local frameDuration = mResting and 0.200 or 0.040
   local frameCount = mResting and 5 or 4
   local firstFrame = mResting and 4 or 0
   local frame = firstFrame + math.floor(mAnimationElapsed / frameDuration) % frameCount
   updateSpriteRect(0, frame * FRAME_SIZE_PX, (mDirection - 1) * FRAME_SIZE_PX, FRAME_SIZE_PX, FRAME_SIZE_PX)
end

function writeProperty(key, value)
   local number = tonumber(value)
   if (number == nil or number ~= number or number == math.huge or number == -math.huge) then
      return
   end

   if (key == "damage" and number >= 0) then
      properties.damage = math.floor(number)
      updateProperties(properties)
      setDamage(properties.damage)
   elseif (mConfig[key] ~= nil and number > 0) then
      mConfig[key] = number
   end
end

function setPath(name, points)
end

function hit(damage)
end

function timeout(id)
end
