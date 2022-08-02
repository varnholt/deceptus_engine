require "data/scripts/enemies/constants"
require "data/scripts/enemies/interpolation"
v2d = require "data/scripts/enemies/vectorial2"


------------------------------------------------------------------------------------------------------------------------
properties = {
   sprite = "data/sprites/enemy_blob_2.png",
   velocity_walk_max = 0.4,
   acceleration_ground = 0.1,
   damage = 40
}


------------------------------------------------------------------------------------------------------------------------
mPatrolTimer = 1
mKeyPressed = 0

SPRITE_WIDTH = 72
SPRITE_HEIGHT = 72

-- idle         row 0    15 sprites
-- left         row 1    16 sprites
-- right        row 2    16 sprites
-- jump up      row 3    12 sprites
-- jump down    row 4    12 sprites
-- die          row 5    12 sprites

ROW_IDLE = 0
ROW_LEFT = 1
ROW_RIGHT = 2
ROW_JUMP_UP = 3
ROW_JUMP_DOWN = 4
ROW_DIE = 5
ROW_DROP = 9

SPRITE_COUNT_IDLE = 15
SPRITE_COUNT_LEFT = 16
SPRITE_COUNT_RIGHT = 16
SPRITE_COUNT_JUMP_UP = 12
SPRITE_COUNT_JUMP_DOWN = 12
SPRITE_COUNT_DIE = 5
SPRITE_COUNT_DROP = 12
SPRITE_COUNT_LANDING = 11 -- last one is skipped, that doesn't make sense

ANIMATION_SPEED = 10.0
IDLE_CYCLE_COUNT = 3

COLLISION_THRESHOLD = 24

mPosition = v2d.Vector2D(0, 0)
mPlayerPosition = v2d.Vector2D(0, 0)
mElapsed = 0.0
mSpriteIndex = 0
mAnimationRow = 0
mIdleCycles = 0
mGravityScale = 1.0
mAlignmentOffset = 0

-- jump related
mJump = false
mJumpStarted = false
mJumpHeightPx = 0
mJumpIntervalMs = 0
mJumpStartPosition = v2d.Vector2D(0, 0)
mJumpTime = 0.0
mJumpPath = {}

-- drop related
mDropping = false
mDropPrepare = false
mDropGravityFlipped = false
mDropTime = 0.0
mDropVelocityExceeded1 = false
mDropPatrolVelocityFactor = 1.0
mDropComplete = false

-- x: 720..792 (30..33 x 24)
-- y: 984 (41 x 24)


------------------------------------------------------------------------------------------------------------------------
-- interpolation keys
SplineKey = {x = 0, y = 0, time = 0}
function SplineKey:create(o)
  o.parent = self
  return o
end


------------------------------------------------------------------------------------------------------------------------
function initialize()
   mPatrolPath = {}
   mPatrolIndex = 1
   mPatrolEpsilon = 1.0
   mWait = false

   addShapeCircle(0.12, 0.0, 0.12)                        -- radius, x, y
   addShapeRect(0.2, 0.07, 0.0, 0.1)                      -- width, height, x, y
   updateSpriteRect(0, 0, 0, SPRITE_WIDTH, SPRITE_HEIGHT) -- id, x, y, width, height
   addHitbox(0, 0, 48, 48)                                -- x offset, y offset, width, height

   -- generate jump path
   k1 = SplineKey:create{x = 0.0, y =  0.0,   time = -1.0}
   k2 = SplineKey:create{x = 0.0, y =  0.01,  time = -0.8}
   k3 = SplineKey:create{x = 0.0, y =  0.07,  time = -0.6}
   k4 = SplineKey:create{x = 0.0, y =  1.0,   time =  0.0}
   k5 = SplineKey:create{x = 0.0, y =  0.0,   time =  0.45}
   k6 = SplineKey:create{x = 0.0, y =  0.0,   time =  1.0}

   mJumpPath = {k1, k2, k3, k4, k5, k6}

   mGood = true
end


------------------------------------------------------------------------------------------------------------------------
function timeout(id)
   -- print(string.format("timeout: %d", id))
   if (id == mPatrolTimer) then
      mWait = false
   end
end


------------------------------------------------------------------------------------------------------------------------
function retrieveProperties()
   updateProperties(properties)
end


------------------------------------------------------------------------------------------------------------------------
function writeProperty(key, value)

   -- print(string.format("write property: %s %s", key, value))

   if (key == "gravity_scale") then
      mGravityScale = tonumber(value)
      setGravityScale(mGravityScale)
      mAlignmentOffset = 6 * 72 - 12
   elseif (key == "jump_height_px") then
      mJump = true
      mJumpHeightPx = value
   elseif (key == "jump_interval_ms") then
      mJumpIntervalMs = value
   end
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
function goLeft()
   spriteIndex = math.floor(math.fmod(mElapsed * ANIMATION_SPEED, SPRITE_COUNT_LEFT))

   if (mSpriteIndex ~= spriteIndex) then

      mSpriteIndex = spriteIndex
      mAnimationRow = ROW_LEFT

      updateSpriteRect(
         0,
         spriteIndex * SPRITE_WIDTH,
         mAnimationRow * SPRITE_HEIGHT + mAlignmentOffset,
         SPRITE_WIDTH,
         SPRITE_HEIGHT
      )

   end

   keyReleased(Key["KeyRight"])
   keyPressed(Key["KeyLeft"])
end


------------------------------------------------------------------------------------------------------------------------
function goRight()
   spriteIndex = math.floor(math.fmod(mElapsed * ANIMATION_SPEED, SPRITE_COUNT_RIGHT))

   if (mSpriteIndex ~= spriteIndex) then

      mSpriteIndex = spriteIndex
      mAnimationRow = ROW_RIGHT

      updateSpriteRect(
         0,
         spriteIndex * SPRITE_WIDTH,
         mAnimationRow * SPRITE_HEIGHT + mAlignmentOffset,
         SPRITE_WIDTH,
         SPRITE_HEIGHT
      )

   end

   keyReleased(Key["KeyLeft"])
   keyPressed(Key["KeyRight"])
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
-- |          o      p          | x
-- |          p      o          | x
function followPlayer()
   local epsilon = 5
   if (mPlayerPosition:getX() > mPosition:getX() + epsilon) then
      goRight()
   elseif (mPlayerPosition:getX() < mPosition:getX() - epsilon) then
      goLeft()
   else
      mKeyPressed = 0
   end
end


------------------------------------------------------------------------------------------------------------------------
function wait()
   -- finish left/right movement animation
   if (mAnimationRow == ROW_LEFT or mAnimationRow == ROW_RIGHT) then
      spriteIndex = math.floor(math.fmod(mElapsed * ANIMATION_SPEED, SPRITE_COUNT_LEFT))
      if (spriteIndex ~= 0) then
         if (spriteIndex ~= mSpriteIndex) then
            mSpriteIndex = spriteIndex
            updateSpriteRect(
               0,
               spriteIndex * SPRITE_WIDTH,
               mAnimationRow * SPRITE_HEIGHT + mAlignmentOffset,
               SPRITE_WIDTH,
               SPRITE_HEIGHT
            )
         end
      else
         mAnimationRow = ROW_IDLE
         mElapsed = 0.0
      end
   else
      -- play idle animation for one cycle
      spriteIndex = math.floor(math.fmod(mElapsed * ANIMATION_SPEED, SPRITE_COUNT_IDLE))

      if (spriteIndex ~= mSpriteIndex) then
         mSpriteIndex = spriteIndex
         updateSpriteRect(
            0,
            spriteIndex * SPRITE_WIDTH,
            mAnimationRow * SPRITE_HEIGHT + mAlignmentOffset,
            SPRITE_WIDTH,
            SPRITE_HEIGHT
         )

         -- after 1 cycle, go back to business
         if (mSpriteIndex == 0) then
            mIdleCycles = mIdleCycles + 1

            -- loop idle animation n times
            if (mIdleCycles == IDLE_CYCLE_COUNT) then
               mWait = false
               mIdleCycles = 0
               mElapsed = 0.0
            end
         end
      end
   end
end


------------------------------------------------------------------------------------------------------------------------
function updatePatrol()
   if (mWait == true) then
      wait()
      return
   end

   local key = mPatrolPath[mPatrolIndex]

   if (key == nil) then
      return
   end

   -- update the speed according to the sprite index
   prop = {velocity_walk_max = mDropPatrolVelocityFactor * (math.sin((mSpriteIndex / SPRITE_COUNT_LEFT) * 6.28318530718) + 1.0) * 0.4}
   updateProperties(prop)

   local keyVec = v2d.Vector2D(key:getX(), key:getY())
   local count = #mPatrolPath

   if     (mPosition:getX() > keyVec:getX() + mPatrolEpsilon) then
      goLeft()
   elseif (mPosition:getX() < keyVec:getX() - mPatrolEpsilon) then
      goRight()
   else
      mWait = true
      mKeyPressed = 0

      mPatrolIndex = mPatrolIndex + 1
      if (mPatrolIndex > count) then
         mPatrolIndex = 0
      end
   end
end


------------------------------------------------------------------------------------------------------------------------
function updateJump(dt)

   if (not mJump) then
      return false
   end

   if (not mJumpStarted) then
      mJumpTime = -1.0
      mJumpStarted = true
   end

   jump_height = 4 * 24
   jump_speed = 1.0

   jump_value = getValueCubic(mJumpPath, mJumpTime):getY()
   if (jump_value < 0.0) then
      jump_value = 0.0
   end

   y = mJumpStartPosition:getY() - jump_height * jump_value + 12;

   setTransform(mPosition:getX(), y, 0.0)

   index = 0
   row = 0

   if (mJumpTime < 0.0) then
      index = math.floor((mJumpTime + 1.0) * (SPRITE_COUNT_JUMP_UP))
   else
      index = math.floor(mJumpTime * SPRITE_COUNT_JUMP_DOWN)
   end

   row = (mJumpTime < 0.0) and ROW_JUMP_UP or ROW_JUMP_DOWN

   updateSpriteRect(
      0,
      index * SPRITE_WIDTH,
      row * SPRITE_HEIGHT,
      SPRITE_WIDTH,
      SPRITE_HEIGHT
   )

   mJumpTime = mJumpTime + dt * jump_speed

   if (mJumpTime >= 1.0) then
      mJumpStarted = false
   end

   return true
end


------------------------------------------------------------------------------------------------------------------------
-- returns true if no subsequent code in the update loop should be executed
function updateDrop(dt)

   if (mDropComplete) then
      return false
   end

   -- check if need to drop
   if (not mDropPrepare and mAlignmentOffset > 0) then

      dx = mPosition:getX() - mPlayerPosition:getX()

      if (dx > -COLLISION_THRESHOLD and dx < COLLISION_THRESHOLD) then

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
               mDropPrepare = true
               return true
            end
         end
      end
   end

   -- check if need to transition sprites to actual drop
   if (mDropPrepare and not mDropping) then

      -- apply a massive slow down on the patrol movement so the blob actually lands on the player
      mDropPatrolVelocityFactor = 0.1

      -- wait until sprite index reaches 0 or 1 so we can transition to drop animation
      if (mSpriteIndex == 0 or mSpriteIndex == 1) then
         mDropping = true
         mDropPrepare = false
      else
         -- keep moving until reached the right sprite
         return false
      end
   end

   -- play drop animation
   if (mDropping) then

      mDropTime = mDropTime + dt

      -- first half of the drop animation:
      -- starting to drop and then falling
      if (not mDropLanded) then
         mAnimationRow = ROW_DROP
         spriteIndex = math.floor(mDropTime * ANIMATION_SPEED)

         -- clamp at end of the column for the whole drop
         if (spriteIndex >= SPRITE_COUNT_DROP) then
            spriteIndex = SPRITE_COUNT_DROP - 1
         end

         if (mSpriteIndex ~= spriteIndex) then

            mSpriteIndex = spriteIndex

            -- interpolate from one to the other sprite offset
            -- should go from -12 to +12 from the point the gravity scale
            -- is inverted (that is sprite index 5).
            -- spriteIndex goes from 0..11
            spriteOffset = 0
            if (spriteIndex >= 5) then
               -- 0..6
               spriteOffset = (spriteIndex - 5) * 4
            end
            updateSpriteRect(
               0,
               spriteIndex * SPRITE_WIDTH,
               mAnimationRow * SPRITE_HEIGHT - (12 - spriteOffset),
               SPRITE_WIDTH,
               SPRITE_HEIGHT
            )

            -- once we reach column 5, the blob is more or less detach from the ceiling,
            -- so now we allow actually letting it fall down
            if (mSpriteIndex == 5) then
               if (not mDropGravityFlipped) then
                  setGravityScale(-mGravityScale)
                  mDropGravityFlipped = true
               end
            end
         end

         -- as soon as our guy decelerates in y to something close to 0,
         -- we know that it has collided with the ground -> time to draw the landing anim
         velocity = getLinearVelocity()

         if (velocity[2] > 1.0) then
            mDropVelocityExceeded1 = true
         end

         if (mDropVelocityExceeded1 and velocity[2] <= 0.01) then
            if (not mDropLanded) then
               mDropLanded = true
               mDropTime = 0.0

               -- reset the slow down and move normally
               mDropPatrolVelocityFactor = 1.0
            end
         end

      else

         mAnimationRow = ROW_JUMP_DOWN
         mAlignmentOffset = 0

         -- the blob landed on the ground, play the landing animation
         -- also skip the first 5 frames because they somewhat don't make sense
         spriteIndex = 5 + math.floor(mDropTime * ANIMATION_SPEED)

         -- once this animation cycle is complete, the whole drop is done
         if (spriteIndex >= SPRITE_COUNT_LANDING) then
            spriteIndex = SPRITE_COUNT_LANDING - 1
            mDropComplete = true
         end

         if (mSpriteIndex ~= spriteIndex) then

            mSpriteIndex = spriteIndex

            updateSpriteRect(
               0,
               spriteIndex * SPRITE_WIDTH,
               mAnimationRow * SPRITE_HEIGHT,
               SPRITE_WIDTH,
               SPRITE_HEIGHT
            )
         end
      end

      return true
   end

   return false
end


------------------------------------------------------------------------------------------------------------------------
function update(dt)
   mElapsed = mElapsed + dt

   if (updateJump(dt)) then
      return
   end

   if (updateDrop(dt)) then
      return
   end

   updatePatrol()
   updateKeysPressed(mKeyPressed)
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
      mPatrolPath = v
   end
end



------------------------------------------------------------------------------------------------------------------------
function setStartPosition(x, y)
   mJumpStartPosition = v2d.Vector2D(x, y)
end
