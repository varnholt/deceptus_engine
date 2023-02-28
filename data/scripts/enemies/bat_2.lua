------------------------------------------------------------------------------------------------------------------------
require "data/scripts/enemies/constants"
require "data/scripts/enemies/interpolation"
v2d = require "data/scripts/enemies/vectorial2"


--  0  .x...............................
--  1  .x...............................
--  2  .................................
--  3  .................................
--  4  .................................
--  5  .................................
--  6  .................................
--  7  .................................
--  8  .................................
--  9  000111222333444555666777888wwwbbb
-- 10  000111222333444555666777888wwwbbb
--
--     012345678901234567890123456789012
--     0  1  2  3  4  5  6  7  8  9  0

-- 221, 77
-- 210, 78
-- 223, 77
-- player: 206, 82

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
   sprite = "data/sprites/enemy_bat_2.png",
   damage = 1
}


------------------------------------------------------------------------------------------------------------------------
mPosition = v2d.Vector2D(0, 0)
mPositionAtDeath = v2d.Vector2D(0, 0)
mPlayerPosition = v2d.Vector2D(0, 0)
mPlayerPositionPrevious = v2d.Vector2D(0, 0)
mElapsed = math.random(0, 3)
mAttackTime = 0
mIdleTime = 0
mActivated = false
mMoveRangeY = 48
mSpriteOffsetX = 0
mSpriteOffsetY = 0 -- 9 * 24
mSpriteWidth = 72
mSpriteHeight = 72
mStartPosition = v2d.Vector2D(0, 0)
mDying = false
mDead = false
mDeathTime = 0
mTransformY = 0
mAttack = false
mExploding = false
mExploded = false
mPath = {}
mEnergy = 3
ANIMATION_SPEED = 40.0
ANIMATION_SPEED_IDLE = 20.0
ANIMATION_SPEED_DEATH = 20.0
HIT_RADIUS = 0.3
ATTACK_DURATION = 1.0
ATTACK_SPRITE_COUNT = 9


------------------------------------------------------------------------------------------------------------------------
function initialize()
   addHitbox(-18, -12, 36, 24)
   addShapeCircle(HIT_RADIUS, 0.0, 0.0)
   updateSpriteRect(
      0,
      0,
      0,
      mSpriteWidth,
      mSpriteHeight
   )
   setZ(30) -- somewhere in the foreground

   addDebugRect()
end


------------------------------------------------------------------------------------------------------------------------
function attack()
   mAttack = true

   mAttackTime = mElapsed

   bx = mPosition:getX()
   by = mPosition:getY()

   px = mPlayerPosition:getX()
   py = mPlayerPosition:getY()

   mSpriteOffsetY = (px > bx) and 0 * 24 or 3 * 24

   sx = mStartPosition:getX()
   sy = mStartPosition:getY()

   k1 = Key:create{x = bx, y = by, time = 0.0}
   k2 = Key:create{x = px, y = py, time = 0.5} -- player pos
   k3 = Key:create{x = sx, y = sy, time = 1.0} -- go back

   mPath = {k1, k2, k3, k4, k5, k6}
end


------------------------------------------------------------------------------------------------------------------------
function moveTo(x, y)
end


------------------------------------------------------------------------------------------------------------------------
function update(dt)

   updateSprite = false
   mElapsed = mElapsed + dt

   -- check if dead
   if (not mDying) then
      if (mEnergy == 0) then
         mDying = true
         mDeathTime = mElapsed
         mSpriteOffsetX = 0
         mSpriteOffsetY = 2 * mSpriteHeight
         setActive(false)
         mPositionAtDeath = mPosition
      end
   end

   if (mDying) then
      -- stickytape the bat to its death location
      setTransform(mPositionAtDeath:getX(), mPositionAtDeath:getY(), 0.0)
   else
      -- consider attacking
      idle = mElapsed < mIdleTime
      if (not mAttack or idle) then

         -- make sure bat is not too far away from player in x
         xDiff = mPosition:getX() // 24 - mPlayerPosition:getX() // 24
         if (math.abs(xDiff) < 6) then

            -- make sure bat is not too far away from player in y
            yDiff = mPosition:getY() // 24 - mPlayerPosition:getY() // 24
            if (yDiff < 0 and yDiff > -7 and not idle) then

               attack()
            end
         end

         -- update transform
         -- mTransformY = 0.25 * math.sin(mElapsed) * mMoveRangeY
         mTransformY = 0
         setTransform(mStartPosition:getX(), mStartPosition:getY() + mTransformY, 0.0)

      -- carry out attack
      else
         time = (mElapsed - mAttackTime) / ATTACK_DURATION
         p = getValueCubic(mPath, time)

         -- print(string.format("i: %f, x: %f, y: %f", time, p:getX(), p:getY()))
         setTransform(p:getX(), p:getY(), 0.0)

         if (time > 1.0) then
            mAttack = false
            mIdleTime = mElapsed + 1.0
         end
      end
   end

   if (mDead) then
      die()
   end

   -- update sprite offset
   if (mDying) then
      -- 12 sprites per row
      spriteIndex = math.min(math.floor((mElapsed - mDeathTime) * ANIMATION_SPEED_DEATH), 21)
      if (spriteIndex == 21) then
         spriteIndex = 20
         mDead = true
      end
   elseif (idle or not mAttack) then
      spriteIndex = math.floor(math.fmod(mElapsed * ANIMATION_SPEED_IDLE, ATTACK_SPRITE_COUNT))
   else
      spriteIndex = math.floor(math.fmod(mElapsed * ANIMATION_SPEED, ATTACK_SPRITE_COUNT))
   end

   -- update sprite index
   if (index ~= mSpriteOffsetX) then
      mSpriteOffsetX = spriteIndex
      updateSprite = true
   end

   updateDebugRect(0, mPosition:getX() - 12, mPosition:getY() + 12, 24, 24)

   if (mExploding and not mExploded and not mDead) then

      -- mExploded = true
      -- mDead = true

      -- |24px|
      -- +----+----+----+ - - -
      -- |    |    |    | 24px
      -- +----+----+----+ - - -
      -- |    |    |    |
      -- +----+----+----+
      -- |    |(XX)|    |
      -- +----+----+----+

      intersects = intersectsWithPlayer(mPosition:getX() - 12, mPosition:getY() + 12, 24, 24)

      if (intersects) then
         playDetonationAnimation(mPosition:getX(), mPosition:getY())
      end
   end

   if (updateSprite) then
      updateSpriteRect(
         0,
         mSpriteOffsetX * mSpriteWidth,
         mSpriteOffsetY,
         mSpriteWidth,
         mSpriteHeight
      ) -- x, y, width, height
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
function setStartPosition(x, y)
   mStartPosition = v2d.Vector2D(x, y)
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
function hit(damage_value)
  mEnergy = mEnergy - 1
end


------------------------------------------------------------------------------------------------------------------------
function writeProperty(key, value)

   -- print(string.format("write property: %s %s", key, value))

   if (key == "exploding") then
      mExploding = true
   end
end
