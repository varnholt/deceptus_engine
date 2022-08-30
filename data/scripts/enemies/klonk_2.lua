------------------------------------------------------------------------------------------------------------------------
require "data/scripts/enemies/constants"
v2d = require "data/scripts/enemies/vectorial2"

------------------------------------------------------------------------------------------------------------------------
properties = {
   static_body = true,
   sprite = "data/sprites/enemy_klonk_2.png",
   damage = 200
}


------------------------------------------------------------------------------------------------------------------------
mPosition = v2d.Vector2D(0, 0)
mPlayerPosition = v2d.Vector2D(0, 0)
mSpriteIndexStone = 0
mSpriteIndexSmoke = 0
mElapsed = 0.0
mCycle = 0
mSpeed = 35.0
mRandomizedOffset = math.random(100)/100.0

COLLISION_THRESHOLD = 24
SPRITE_SIZE_PX = 4 * 24
CYCLE_IDLE = 0
CYCLE_WAKE = 1
CYCLE_STEADY = 2
CYCLE_FALLING = 3
CYCLE_COLLIDE = 4
CYCLE_DUST_BOTTOM_LEFT = 10
CYCLE_DUST_BOTTOM_RIGHT = 11
TILE_COUNT_IDLE = 3
TILE_COUNT_WAKE = 12
TILE_COUNT_STEADY = 12
TILE_COUNT_FALLING = 1
TILE_COUNT_COLLIDE = 12


------------------------------------------------------------------------------------------------------------------------
function initialize()

   box_size = 0.48
   box_size_origin = 0.5 - box_size

   addShapeRect(0.48, 0.48, box_size_origin, box_size_origin)
   addSprite() -- for dust left
   addSprite() -- for dust right
   updateSpriteRect(0, 0, 0, SPRITE_SIZE_PX, SPRITE_SIZE_PX) -- id, x, y, width, height
   updateSpriteRect(1, 0, 0, 1, 1)
   updateSpriteRect(2, 0, 0, 1, 1)
   setSpriteOffset(1, -38, -21)
   setSpriteOffset(2, 38, -21)
end


------------------------------------------------------------------------------------------------------------------------
function update(dt)

   -- make sure block is on same x as player
   if (mCycle == CYCLE_IDLE) then

      mElapsed = mElapsed + dt
      mSpriteIndexStone = math.floor(math.fmod(mRandomizedOffset + mElapsed, 3))

      updateSpriteRect(
         0,
         mSpriteIndexStone * SPRITE_SIZE_PX,
         CYCLE_IDLE * SPRITE_SIZE_PX,
         SPRITE_SIZE_PX,
         SPRITE_SIZE_PX
      )

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
               -- activate the stone
               mElapsed = 0.0
               mCycle = CYCLE_WAKE
            end
         end
      end
   end

   -- stone wakes up
   if (mCycle == CYCLE_WAKE) then

      mElapsed = mElapsed + dt
      mSpriteIndexStone = math.floor(mElapsed * mSpeed)

      -- maybe close eyes again if player is out of sight?
      if (mSpriteIndexStone < 12) then
         updateSpriteRect(
            0,
            mSpriteIndexStone * SPRITE_SIZE_PX, 2 * SPRITE_SIZE_PX,
            SPRITE_SIZE_PX,
            SPRITE_SIZE_PX
         )
      else
         makeDynamic()
         mCycle = CYCLE_STEADY
      end

   end

   -- block starts falling
   if (mCycle == CYCLE_STEADY) then
      velocity = getLinearVelocity()

      if (velocity[2] > 1.0) then
         mCycle = CYCLE_FALLING
      end
   end

   -- stone hit the floor
   if (mCycle == CYCLE_FALLING) then
      velocity = getLinearVelocity()

      if (velocity[2] <= 0.01) then
         velocity = getLinearVelocity()
         mCycle = CYCLE_COLLIDE
         setActive(false)
         setDamage(0)
         boom(0.0, 1.0, 0.5)
         mElapsed = 0.0
      end
   end

   -- stone hit floor animation
   if (mCycle == CYCLE_COLLIDE) then

       mElapsed = mElapsed + dt
       mSpriteIndexStone = math.floor(mElapsed * mSpeed)
       mSpriteIndexSmoke = math.floor(mElapsed * mSpeed)

       if (mSpriteIndexStone < TILE_COUNT_COLLIDE) then
          updateSpriteRect(0, mSpriteIndexStone * SPRITE_SIZE_PX, CYCLE_COLLIDE * SPRITE_SIZE_PX, SPRITE_SIZE_PX, SPRITE_SIZE_PX)
       end

      updateSpriteRect(
         1,
         mSpriteIndexSmoke * SPRITE_SIZE_PX,
         CYCLE_DUST_BOTTOM_LEFT * SPRITE_SIZE_PX,
         SPRITE_SIZE_PX,
         SPRITE_SIZE_PX
      )

      updateSpriteRect(
         2,
         mSpriteIndexSmoke * SPRITE_SIZE_PX,
         CYCLE_DUST_BOTTOM_RIGHT * SPRITE_SIZE_PX,
         SPRITE_SIZE_PX,
         SPRITE_SIZE_PX
      )
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

