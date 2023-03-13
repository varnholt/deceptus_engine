------------------------------------------------------------------------------------------------------------------------
-- place cannons with a 48x24 rectangle in your level


------------------------------------------------------------------------------------------------------------------------
require "data/scripts/enemies/constants"
v2d = require "data/scripts/enemies/vectorial2"


------------------------------------------------------------------------------------------------------------------------
properties = {
   collides_with_player = true,
   static_body = true,
   sprite = "data/sprites/enemy_pirate_cannon.png",
   damage = 0,
   walk_through = false
}


------------------------------------------------------------------------------------------------------------------------
mFireInterval = 3.0
mReadytoFire = true
mPosition = v2d.Vector2D(0, 0)
mPlayerPosition = v2d.Vector2D(0, 0)
mSpriteIndex = 0
mElapsed = 0.0
mElapsedSinceFired = 0.0
mElapsedUntilFired = 0.0
mX = -1.0
mAlignmentOffset = 0
mSpeed = 1.5
mProjectileIndex = 0

mIdle = true
mFired = false

SPRITE_WIDTH = 6 * 24
SPRITE_HEIGHT = 3 * 24


------------------------------------------------------------------------------------------------------------------------
function initialize()
   -- addShapeRect(0.5, 0.25, 0.5, 0.25) -- width, height, x, y
   addShapeRectBevel(1.0, 0.75, 0.05, 0.0, 0.25)

   -- set up boom and audio distance
   addHitbox(0, 0, 48, 48)
   addAudioRange(400.0, 0.0, 200.0, 1.0)
   addSample("mechanism_cannon_1.wav")
   addSample("mechanism_cannon_2.wav")
   addSample("mechanism_cannon_3.wav")
   addSample("mechanism_cannon_4.wav")
   addSample("mechanism_cannon_boom_1.wav")
   addSample("mechanism_cannon_boom_2.wav")

   addWeapon(WeaponType["Gun"], 1000, 60, 0.2) -- interval, damage, radius

   registerHitAnimation(
      0,
      "data/sprites/enemy_pirate_cannon_cannonball.png",
      3 * 24,
      3 * 24,
      0.05,
      20,
      24,
      4
   )

   registerHitSamples(
      "data/sprites/enemy_pirate_cannon_cannonball.png",
      "mechanism_cannon_boom_1.wav",
      0.5,
      "mechanism_cannon_boom_2.wav",
      0.5
   )

   updateProjectileAnimation(
      0,
      "data/sprites/enemy_pirate_cannon_cannonball.png",
      3 * 24,
      3 * 24,
      (3 * 24) / 2,
      (3 * 24) / 2,
      0.05,
      4,
      15,
      0
   )

   setSpriteOffset(0, 24, 12);
end


------------------------------------------------------------------------------------------------------------------------
function writeProperty(key, value)
   if (key == "alignment") then
      if (value == "right") then
         -- print("setting alignment to left")
         mX = 1.0
         mAlignmentOffset = 5 * SPRITE_HEIGHT
      end
   elseif (key == "time_offset_s") then
      mElapsedUntilFired = mElapsedUntilFired + value
   elseif (key == "fire_interval_s") then
      mFireInterval = value
   end
end


------------------------------------------------------------------------------------------------------------------------
function timeout(id)
end


------------------------------------------------------------------------------------------------------------------------
function fire()
   playSample(string.format("mechanism_cannon_%d.wav", math.random(1, 4)), 0.5)

   useGun(
      0,
      mPosition:getX() + mX * 32,
      mPosition:getY() + 24,
      mX * mSpeed,
      0.0
   );
end


------------------------------------------------------------------------------------------------------------------------
function retrieveProperties()
   updateProperties(properties)
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
function update(dt)

   mElapsedUntilFired = mElapsedUntilFired + dt
   if (mElapsedUntilFired > mFireInterval) then
      mElapsedUntilFired = 0
      mReadytoFire = true
   end

   -- if timer elapsed, go from idle animation to fire animation
   if (mReadytoFire) then
      mReadytoFire = false
      mIdle = false
      mElapsed = 0
   end

   -- update sprite index
   mElapsed = mElapsed + dt
   index = mSpriteIndex
   mSpriteIndex = math.floor(mElapsed * 20.0)

   -- update projectile index
   projectileIndex = 0
   if (mFired) then
      mElapsedSinceFired = mElapsedSinceFired + dt
      projectileIndex = math.floor(mElapsedSinceFired * 5.0)
      projectileIndex = math.min(3, projectileIndex)
   end

   col = 0
   row = 0

   -- idle animation
   if (mIdle) then
      col = mSpriteIndex % 3
      row = 0

   -- fire animation
   else
      col = mSpriteIndex % 12
      row = 1 + (math.floor(mSpriteIndex / 12)) % 4

      -- fire the ball at the right sprite index
      if (col == 5 and row == 2) then
         fire()
         mFired = true
      end

      -- update projectile index after the cannon has been fired
      if (mFired) then
         if (projectileIndex ~= mProjectileIndex) then
            mProjectileIndex = projectileIndex
         end
      end

      -- animation is done, go to idle animation
      if (col == 3 and row == 4) then
         mIdle = true
         mFired = false
         mElapsedSinceFired = 0
      end
   end

   -- print(string.format("col: %f, row: %f", col, row))

   if (index ~= mSpriteIndex) then
      updateSpriteRect(
         0,
         col * SPRITE_WIDTH,
         row * SPRITE_HEIGHT + mAlignmentOffset,
         SPRITE_WIDTH,
         SPRITE_HEIGHT
      )
   end

end


------------------------------------------------------------------------------------------------------------------------
function setPath(name, table)
end

