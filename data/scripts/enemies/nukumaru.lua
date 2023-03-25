require "data/scripts/enemies/constants"

------------------------------------------------------------------------------------------------------------------------
properties = {
   sprite = "data/sprites/enemy_nukumaru.png",
   damage = 40,
   restitution = 1.0, -- maximum bounce
   density = 0.01
}

SPRITE_WIDTH = 128
SPRITE_HEIGHT = 128
SPRITE_COUNT = 60
ANIMATION_SPEED = 100.0

_elapsed = 0.0
_tick_count = 0
_sprite_index = -1
_impulse_x = 0.03
_impulse_y = 0.03

------------------------------------------------------------------------------------------------------------------------
function initialize()

   addShapeCircle(0.6, 0.0, -0.06)                        -- radius, x, y
   updateSpriteRect(0, 0, 0, SPRITE_WIDTH, SPRITE_HEIGHT) -- id, x, y, width, height
   addHitbox(-18, -18, 36, 36)                            -- x offset, y offset, width, height
   setZ(30)                                               -- place in foreground
   setGravityScale(0.0)                                   -- nukumaru is not impacted by gravity
end

------------------------------------------------------------------------------------------------------------------------
function update(dt)
   _elapsed = _elapsed + dt

   -- wait just a tiny bit before applying that impulse
   _tick_count = _tick_count + 1
   if (_tick_count == 10) then
      applyLinearImpulse(_impulse_x, _impulse_y)
   end

   -- update sprite
   sprite_index = math.floor(math.fmod(_elapsed * ANIMATION_SPEED, SPRITE_COUNT))
   if (sprite_index ~= _sprite_index) then
      _sprite_index = sprite_index
      updateSpriteRect(
         0,
         _sprite_index * SPRITE_WIDTH,
         0,
         SPRITE_WIDTH,
         SPRITE_HEIGHT
      ) -- id, x, y, width, height
   end
end

------------------------------------------------------------------------------------------------------------------------
function writeProperty(key, value)
   if (key == "impulse_x") then
      _impulse_x = value
   elseif (key == "impulse_y") then
      _impulse_y = value
   end
end

------------------------------------------------------------------------------------------------------------------------
function retrieveProperties()
   updateProperties(properties)
end
