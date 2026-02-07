require "data/scripts/enemies/constants"
v2d = require "data/scripts/enemies/vectorial2"

-- frog = 2 x 2 x 24
--
-- +---+---+
-- | xx|xx |
-- +---+---+
-- | xx|xx |
-- +---+---+
-- 0 .... 48

--  7px offset to bottom
-- 10px offset to top
--  3px offset to sides

-- sprite rows:
-- 1) idle left (8 frames)
-- 2) idle blink left (8 frames)
-- 3) attack left ( 6 frames)
-- 4) idle right (8 frames)
-- 5) idle blink right (8 frames)
-- 6) attack right (6 frames)
-- 
-- his tongue can be extended by using the sprites next to rows 3 and 6

CYCLE_IDLE = 1
CYCLE_BLINK = 2
CYCLE_ATTACK = 3
ROW_OFFSET_RIGHT = 3
FRAME_COUNTS = {8, 8, 6}
TONGUE_PART_1 = {288, 96} -- height: 48px, width: 24px
TONGUE_PART_2 = {312, 96} -- height: 48px, width: 24px

SPRITE_WIDTH = 48
SPRITE_HEIGHT = 48

STATE_IDLE = 1
STATE_ATTACK = 2


------------------------------------------------------------------------------------------------------------------------
properties = {
   static_body = true,
   sprite = "data/sprites/enemy_frog.png",
   damage = 0,
   sensor = true
}


------------------------------------------------------------------------------------------------------------------------
mDone = false
mPosition = v2d.Vector2D(0, 0)
mPlayerPosition = v2d.Vector2D(0, 0)
mElapsed = 0
mAlignmentOffset = 0
mState = STATE_IDLE
mPointsLeft = true


------------------------------------------------------------------------------------------------------------------------
function updateState()

   xDiff = mPlayerPosition:getX() // 24 - mPosition:getX() // 24
   yDiff = mPlayerPosition:getY() // 24 - mPosition:getY() // 24

   xInRange = (mPointsLeft and xDiff >= -5 and xDiff <= 0) or (not mPointsLeft and xDiff <= 5 and xDiff >= 0)
   yInRange = yDiff >= -1 and yDiff <= 1

   nextState = STATE_IDLE

   -- check for attack transition
   if (yInRange and xInRange) then
      if (
         isPhsyicsPathClear(
            mPosition:getX(),
            mPosition:getY(),
            mPlayerPosition:getX(),
            mPlayerPosition:getY()
         )
      )
      then
         print(xDiff)
         print("attack")
         nextState = STATE_ATTACK
      end
   end

   if (nextState ~= mStatateNeeded) then
      mState = nextState
   end

end

------------------------------------------------------------------------------------------------------------------------
function updateSprite(dt)

   x = 0
   y = 0

   updateSpriteRect(
      0,
      x,
      y + mAlignmentOffset,
      SPRITE_WIDTH,
      SPRITE_HEIGHT
   )

end


------------------------------------------------------------------------------------------------------------------------
function initialize()
   addShapeRect(0.2, 0.2, 0.0, 0.1)
   updateSprite(0.0)
   print("frog.lua initialized")
end


------------------------------------------------------------------------------------------------------------------------
function timeout(id)
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

   if (mDone) then
      return
   end

   mPlayerPosition = v2d.Vector2D(x, y)
   distanceToPlayer = (mPlayerPosition - mPosition):getLength()
end


------------------------------------------------------------------------------------------------------------------------
function update(dt)
   mElapsed = mElapsed + dt
   updateState()
   updateSprite(dt)
end


------------------------------------------------------------------------------------------------------------------------
function setPath(name, table)
end


------------------------------------------------------------------------------------------------------------------------
function writeProperty(key, value)
   if (key == "alignment") then
      if (value == "right") then
         mAlignmentOffset = 3 * SPRITE_HEIGHT
         mPointsLeft = false
      end
   end
end

