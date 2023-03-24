require "data/scripts/enemies/constants"
require "data/scripts/enemies/interpolation"
v2d = require "data/scripts/enemies/vectorial2"


------------------------------------------------------------------------------------------------------------------------
properties = {
   sprite = "data/sprites/enemy_nukumaru.png",
   damage = 40,
   restitution = 1.0,
   density = 0.01
}


SPRITE_WIDTH = 128
SPRITE_HEIGHT = 128

_position = v2d.Vector2D(0, 0)
_player_position = v2d.Vector2D(0, 0)
_elapsed = 0.0
_initialized_impulse = false
_impulses = 0


------------------------------------------------------------------------------------------------------------------------
function initialize()

   addShapeCircle(0.8, 0.0, -0.06)                        -- radius, x, y
   updateSpriteRect(0, 0, 0, SPRITE_WIDTH, SPRITE_HEIGHT) -- id, x, y, width, height

   -- this will need a few hitboxes to be more or less accurate
   --
   addHitbox(-18, -18, 36, 36)                            -- x offset, y offset, width, height

   setGravityScale(0.0)

end


------------------------------------------------------------------------------------------------------------------------
function retrieveProperties()
   updateProperties(properties)
end


------------------------------------------------------------------------------------------------------------------------
function writeProperty(key, value)
end


------------------------------------------------------------------------------------------------------------------------
function movedTo(x, y)
   _position = v2d.Vector2D(x, y)
end


------------------------------------------------------------------------------------------------------------------------
function playerMovedTo(x, y)
   _player_position = v2d.Vector2D(x, y)
end


------------------------------------------------------------------------------------------------------------------------
function update(dt)
   _elapsed = _elapsed + dt

   if (_impulses < 20) then
      applyForce(2.0, 0.0)
      -- _initialized_impulse = true
      _impulses = _impulses + 1
   end
end


------------------------------------------------------------------------------------------------------------------------
function setStartPosition(x, y)
   _jump_start_position = v2d.Vector2D(x, y)
end

