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
   sprite = "data/sprites/enemy_frog.png",
   damage = 4,
   smash = true
}


------------------------------------------------------------------------------------------------------------------------
_done = false
_position = v2d.Vector2D(0, 0)
_player_position = v2d.Vector2D(0, 0)
_elapsed = 0
_alignment_offset = 0
_state = STATE_IDLE
_points_left = true
_smashed = false


------------------------------------------------------------------------------------------------------------------------
function updateState()

   x_diff = _player_position:getX() // 24 - _position:getX() // 24
   y_diff = _player_position:getY() // 24 - _position:getY() // 24

   x_in_range =
      (_points_left and x_diff >= -5 and x_diff <= 0) or
      (not _points_left and x_diff <= 5 and x_diff >= 0)

   y_in_range = y_diff >= -1 and y_diff <= 1

   next_state = STATE_IDLE

   -- check for attack transition
   if (y_in_range and x_in_range) then
      if (
         isPhsyicsPathClear(
            _position:getX(),
            _position:getY(),
            _player_position:getX(),
            _player_position:getY()
         )
      ) then
         print(x_diff)
         print("attack")
         next_state = STATE_ATTACK
      end
   end

   if (next_state ~= mStatateNeeded) then
      _state = next_state
   end

end

------------------------------------------------------------------------------------------------------------------------
function updateSprite(dt)

   x = 0
   y = 0

   updateSpriteRect(
      0,
      x,
      y + _alignment_offset,
      SPRITE_WIDTH,
      SPRITE_HEIGHT
   )

end

------------------------------------------------------------------------------------------------------------------------
function smashed()

   if (_smashed) then
      return
   end

   _smashed = true
   startDying()
end

------------------------------------------------------------------------------------------------------------------------
function initialize()
   addShapeRect(0.4, 0.25, 0.0, 0.05)
   -- setSpriteOffset(0, 0, 24);
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
   _position = v2d.Vector2D(x, y)
end


------------------------------------------------------------------------------------------------------------------------
function playerMovedTo(x, y)

   if (_done) then
      return
   end

   _player_position = v2d.Vector2D(x, y)
   distance_to_player = (_player_position - _position):getLength()
end


------------------------------------------------------------------------------------------------------------------------
function update(dt)
   _elapsed = _elapsed + dt
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
         _alignment_offset = 3 * SPRITE_HEIGHT
         _points_left = false
      end
   end
end
