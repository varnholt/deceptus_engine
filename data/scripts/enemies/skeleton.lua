
require "data/scripts/enemies/constants"
require "data/scripts/enemies/helpers"
v2d = require "data/scripts/enemies/vectorial2"

-- enemy configuration
properties = {
   sprite = "data/sprites/enemy_skeleton.png",
   velocity_walk_max = 1.0,
   acceleration_ground = 0.1
}

-- those are in sprite sheet order
Action = {
   Walk =   0,
   Idle =   1,
   Hit =    2,
   Die =    3,
   Attack = 4
}


------------------------------------------------------------------------------------------------------------------------
mPatrolTimer = 1
mKeyPressed = 0
mPosition = v2d.Vector2D(0, 0)
mPlayerPosition = v2d.Vector2D(0, 0)
mPointsLeft = false
mSpriteTime = math.random(0, 3)
mTime = 0
mSpriteIndex = 0
mSpriteCounts = {12, 10, 6, 14, 14}
mSpriteOffsets = {0, 144, 288, 432, 576}
mCurrentAction = Action["Idle"]
mEnergy = 100
mDead = false
mWaiting = false
mAttackStarted = false
mAttackLaunched = false
mHit = false


------------------------------------------------------------------------------------------------------------------------
function initialize()
   mPatrolPath = {}
   mPatrolIndex = 1
   mPatrolEpsilon = 1.0
   addShapeRect(0.2, 0.5, 0.0, 0.23)
   updateSpriteRect(0, 0, 72, 72)
end


------------------------------------------------------------------------------------------------------------------------
function timeout(id)
   -- print(string.format("timeout: %d", id))
   if (id == mPatrolTimer) then
      mWaiting = false
   end
end


------------------------------------------------------------------------------------------------------------------------
function retrieveProperties()
   updateProperties(properties)
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
   mPointsLeft = true
   keyReleased(Key["KeyRight"])
   keyPressed(Key["KeyLeft"])
end


------------------------------------------------------------------------------------------------------------------------
function goRight()
   mPointsLeft = false
   keyReleased(Key["KeyLeft"])
   keyPressed(Key["KeyRight"])
end


------------------------------------------------------------------------------------------------------------------------
function movedTo(x, y)
   -- print(string.format("moved to: %f, %f", x, y))
   mPosition = v2d.Vector2D(x, y)
end


------------------------------------------------------------------------------------------------------------------------
function playerMovedTo(x, y)
   -- print(string.format("player moved to: %f, %f", x, y))
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

   if (mWaiting) then
      return
   end

   mWaiting = true

   local count = #mPatrolPath
   mKeyPressed = 0
   timer(3000, mPatrolTimer)
   mPatrolIndex = mPatrolIndex + 1

   if (mPatrolIndex > count) then
      mPatrolIndex = 0
   end

end


------------------------------------------------------------------------------------------------------------------------
function patrol()

   local key = mPatrolPath[mPatrolIndex]
   local keyVec = v2d.Vector2D(key:getX(), key:getY())

   if     (mPosition:getX() > keyVec:getX() + mPatrolEpsilon) then
      goLeft()
   elseif (mPosition:getX() < keyVec:getX() - mPatrolEpsilon) then
      goRight()
   else
      wait()
   end

end


------------------------------------------------------------------------------------------------------------------------
function walk()

   if (isPlayerInReach()) then
      followPlayer()
   else
      patrol()
   end

end


------------------------------------------------------------------------------------------------------------------------
function act()

   if (mCurrentAction == Action["Hit"]) then
      updateHit()
   elseif (mCurrentAction == Action["Die"]) then
      updateDead()
   elseif (mCurrentAction == Action["Walk"]) then
      walk()
   elseif (mCurrentAction == Action["Attack"]) then
      attack()
   end

end


------------------------------------------------------------------------------------------------------------------------
function updateDead()

   if (mSpriteIndex == mSpriteCounts[Action["Die"] + 1] - 1) then

      die()

   end

end


------------------------------------------------------------------------------------------------------------------------
function attack()

   -- print(mTime)

   mKeyPressed = 0
   mAttackStarted = true

   if (mSpriteIndex == 6) then
      if (not mAttackLaunched) then
         mAttackLaunched = true
         damage(50, mPointsLeft and -5000.0 or 5000.0, 0.0)
      end
   else

      if (mSpriteIndex == mSpriteCounts[Action["Attack"]] -1) then
         mAttackStarted = false
      end

      mAttackLaunched = false
   end

end


------------------------------------------------------------------------------------------------------------------------
function isPlayerInReach()

   inReach = false

   -- check if player is within range
   distanceToPlayerX = mPosition:getX() // 24 - mPlayerPosition:getX() // 24

   if (math.abs(distanceToPlayerX) < 10) then

      distanceToPlayerY = mPosition:getY() // 24 - mPlayerPosition:getY() // 24

      inReach = true

      -- if (math.abs(distanceToPlayerY) == 0) then
      --    if (
      --       isPhsyicsPathClear(
      --          mPosition:getX(),
      --          mPosition:getY(),
      --          mPlayerPosition:getX(),
      --          mPlayerPosition:getY()
      --       )
      --    ) then
      --       inReach = true
      --    end
      -- end
   end

   return inReach

end


------------------------------------------------------------------------------------------------------------------------
function hit(damage_value)

   if (not mHit) then

      mHit = true
      mKeyPressed = 0

      -- need to store the current hit time
      -- print(string.format("hit: %d", damage_value))

      mEnergy = mEnergy - damage_value

      if (mEnergy <= 0) then
         mDead = true
      end

   end

end


------------------------------------------------------------------------------------------------------------------------
function updateHit()

   if (mHit) then

      if (mSpriteIndex >= mSpriteCounts[Action["Hit"] + 1] - 1) then
         mHit = false
      end

   end

end


------------------------------------------------------------------------------------------------------------------------
function isHit()
   return mHit
end


------------------------------------------------------------------------------------------------------------------------
function isWaiting()
   return mWaiting
end


------------------------------------------------------------------------------------------------------------------------
function canAttack()
   can = false

   -- if an attack has started, finish the attack
   if (mAttackStarted) then
      return true
   end

   -- check if player is within range
   distanceToPlayerX = (mPosition:getX() - mPlayerPosition:getX()) / 24.0

   -- print(distanceToPlayerX)

   if (math.abs(distanceToPlayerX) <= 1.5) then

      distanceToPlayerY = mPosition:getY() // 24 - mPlayerPosition:getY() // 24

      -- print(distanceToPlayerY)

      if (math.abs(distanceToPlayerY) <= 1) then
         can = true
      end

   end

   return can
end


------------------------------------------------------------------------------------------------------------------------
function think()

   nextAction = mCurrentAction

   -- determine next action

   if (mDead) then
      nextAction = Action["Die"]
   elseif (canAttack()) then
      nextAction = Action["Attack"]
   elseif (isHit()) then
      nextAction = Action["Hit"]
   elseif (not isWaiting()) then
      nextAction = Action["Walk"]
   else
      nextAction = Action["Idle"]
   end

   changed = nextAction ~= mCurrentAction

   -- if (changed) then
   --    print(string.format("%d -> %d", mCurrentAction, nextAction))
   -- end

   updateSprite(changed)
   mCurrentAction = nextAction

end


------------------------------------------------------------------------------------------------------------------------
function updateSprite(changed)

   spriteCount = mSpriteCounts[mCurrentAction + 1]
   spriteOffset = mSpriteOffsets[mCurrentAction + 1]

   updateRequired = false
   pointsLeftPrev = mPointsLeft

   -- if action changed, reset sprite index to 0
   if (changed) then
      spriteIndex = 0
      updateRequired = true
      mSpriteTime = 0
   else
      spriteIndex = math.floor(math.fmod(mSpriteTime * 15.0, spriteCount))
   end

   -- update sprite index
   if (spriteIndex ~= mSpriteIndex) then
      mSpriteIndex = spriteIndex
      updateRequired = true
   end

   if (pointsLeftPrev ~= mPointsLeft) then
      updateRequired = true
   end

   if (updateRequired) then
      updateSpriteRect(
         mSpriteIndex * 72,
         spriteOffset + (mPointsLeft and 72 or 0),
         72,
         72
      ) -- x, y, width, height
   end

end


------------------------------------------------------------------------------------------------------------------------
function update(dt)

   mTime = mTime + dt
   mSpriteTime = mSpriteTime + dt -- that one might get reset

   think()
   act()

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

   if (name == "patrol_path") then
      mPatrolPath = v
   end
end



