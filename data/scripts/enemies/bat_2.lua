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
   staticBody = false,
   sprite = "data/sprites/enemy_bat_2.png",
   damage = 20
}


------------------------------------------------------------------------------------------------------------------------
mPosition = v2d.Vector2D(0, 0)
mPlayerPosition = v2d.Vector2D(0, 0)
mPlayerPositionPrevious = v2d.Vector2D(0, 0)
mPlayerPositionSkip = 0
mElapsed = math.random(0, 3)
mAttackTime = 0
mIdleTime = 0
mActivated = false
mMoveRangeY = 48
mSpriteOffsetX = 0
mSpriteOffsetY = 9 * 24
mSpriteWidth = 72
mSpriteHeight = 72
mStartPosition = v2d.Vector2D(0, 0)
mDone = false
mTransformY = 0
mAttack = false
mPath = {}
ANIMATION_SPEED = 12.0
HIT_RADIUS = 0.3
ATTACK_DURATION = 3.0
ATTACK_SPRITE_COUNT = 9


------------------------------------------------------------------------------------------------------------------------
function initialize()
   addShapeCircle(HIT_RADIUS, 0.0, 0.0)
   updateSpriteRect(
      0,
      0,
      0,
      mSpriteWidth,
      mSpriteHeight
   )
   setZ(30) -- somewhere in the foreground
end


------------------------------------------------------------------------------------------------------------------------
function attack()
   mAttack = true

   mAttackTime = mElapsed

   -- print("attack")

   bx = mPosition:getX()
   by = mPosition:getY()

   px0 = mPlayerPositionPrevious:getX()
   py0 = mPlayerPositionPrevious:getY()

   px1 = mPlayerPosition:getX()
   py1 = mPlayerPosition:getY()

   dx = mPlayerPosition:getX() - mPlayerPositionPrevious:getX()
   dy = mPlayerPosition:getY() - mPlayerPositionPrevious:getY()

   -- clamp dx, dy
   max_delta = 24
   if (dx > max_delta) then
      dx = max_delta
   elseif (dx < -max_delta) then
      dx = -max_delta
   end

   if (dy > max_delta) then
      dy = max_delta
   elseif (dy < -max_delta) then
      dy = -max_delta
   end

   -- estimate new player position
   px2 = px1 + dx * 3.0
   py2 = py1 + dy * 3.0

   sx = mStartPosition:getX()
   sy = mStartPosition:getY()

   k1 = Key:create{x = bx,  y = by,  time = 0.0}
   k2 = Key:create{x = px0, y = py0, time = 0.3} -- right of player
   k3 = Key:create{x = px1, y = py1, time = 0.5} -- player pos
   k4 = Key:create{x = px2, y = py2, time = 0.7} -- left of player
   k5 = Key:create{x = sx,  y = sy,  time = 1.0} -- go back

   mPath = {k1, k2, k3, k4, k5}
end


------------------------------------------------------------------------------------------------------------------------
function update(dt)

   updateSprite = false

   -- get sprite index
   mElapsed = mElapsed + dt

   idle = mElapsed < mIdleTime

   -- make sure block is on same x as player
   if (not mAttack or idle) then
      xDiff = mPosition:getX() // 24 - mPlayerPosition:getX() // 24
      if (math.abs(xDiff) < 5) then

         -- make sure stone is not too far away (10 tiles) and above player
         yDiff = mPosition:getY() // 24 - mPlayerPosition:getY() // 24

         if (yDiff < 0 and yDiff > -10 and not idle) then
            attack()
         end
      end

      -- update transform
      -- mTransformY = 0.25 * math.sin(mElapsed) * mMoveRangeY
      mTransformY = 0
      setTransform(mStartPosition:getX(), mStartPosition:getY() + mTransformY, 0.0)
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

   spriteIndex = math.floor(math.fmod(mElapsed * ANIMATION_SPEED, ATTACK_SPRITE_COUNT))

   -- update sprite index
   if (index ~= mSpriteOffsetX) then
      mSpriteOffsetX = spriteIndex
      updateSprite = true
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

   if (mDone) then
      die()
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
   -- print(string.format("object position: %f, %f", x, y))
   mStartPosition = v2d.Vector2D(x, y)
end


------------------------------------------------------------------------------------------------------------------------
function movedTo(x, y)
   mPosition = v2d.Vector2D(x, y)
end


------------------------------------------------------------------------------------------------------------------------
function playerMovedTo(x, y)
   mPlayerPosition = v2d.Vector2D(x, y)

   mPlayerPositionSkip = mPlayerPositionSkip + 1

   -- store a player position that's 30 frames old
   if (math.floor(math.fmod(mPlayerPositionSkip, 30)) == 0) then
      mPlayerPositionPrevious = mPlayerPosition
      -- print("update")
   end
end


