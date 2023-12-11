require "data/scripts/enemies/constants"

------------------------------------------------------------------------------------------------------------------------
properties = {
   sprite = "data/sprites/enemy_shadow.png"
}

-- sprite width: 3 * 24
-- sprite height: 5 * 24
-- row 1: idle, 20 sprites
-- row 2: blink, 20 sprites
-- row 3: appear 24 sprites

SPRITE_WIDTH = 72
SPRITE_HEIGHT = 120
SPRITE_COUNTS = {20, 20, 24}
ANIMATION_SPEEDS = {13.0, 13.0, 50.0}
ROW_IDLE = 1
ROW_BLINK = 2
ROW_APPEAR = 3

_current_cycle = ROW_IDLE
_elapsed = 0.0
_tick_count = 0
_sprite_index = -1

_shown = false
_animation_dir_forward = true


------------------------------------------------------------------------------------------------------------------------
function initialize()
   addShapeRect(0.75, 1.25, 0.75, 0.0)
   updateSpriteRect(0, 0, 0, SPRITE_WIDTH, SPRITE_HEIGHT) -- id, x, y, width, height
   setSpriteOrigin(0, -SPRITE_WIDTH/2, 0)
   setZ(19)                                               -- place behind player
end

------------------------------------------------------------------------------------------------------------------------
function updateCycle()

   if (_current_cycle == ROW_APPEAR) then
      if (_animation_dir_forward) then
         if (sprite_index == SPRITE_COUNTS[_current_cycle] - 1) then
            idle()
         end
      else
         if (sprite_index == 0) then
            setVisible(false)
         end
      end
   elseif (_current_cycle == ROW_IDLE) then
      if (sprite_index == SPRITE_COUNTS[_current_cycle] - 1) then
         dice = math.random(0, 3)
         if (dice == 0) then
            blink()
         end
      end
   elseif (_current_cycle == ROW_BLINK) then
      if (sprite_index == SPRITE_COUNTS[_current_cycle] - 1) then
         idle()
      end
   end
end

------------------------------------------------------------------------------------------------------------------------
function show()
   if (not _shown) then
      _shown = true
      _elapsed = 0
      _current_cycle = ROW_APPEAR
      sprite_index = 0
      _animation_dir_forward = true
   end
end

------------------------------------------------------------------------------------------------------------------------
function hide()
   _elapsed = 0
   _current_cycle = ROW_APPEAR
   sprite_index = 0
   _animation_dir_forward = false
end

------------------------------------------------------------------------------------------------------------------------
function idle()
   _elapsed = 0
   _current_cycle = ROW_IDLE
   _animation_dir_forward = true
end

------------------------------------------------------------------------------------------------------------------------
function blink()
   _elapsed = 0
   _current_cycle = ROW_BLINK
   _animation_dir_forward = true
end

------------------------------------------------------------------------------------------------------------------------
function update(dt)
   _elapsed = _elapsed + dt

   updateCycle()

   sprite_index = 0

   if (_animation_dir_forward) then
      sprite_index = math.floor(math.fmod(_elapsed * ANIMATION_SPEEDS[_current_cycle], SPRITE_COUNTS[_current_cycle]))
   else
      sprite_index = SPRITE_COUNTS[_current_cycle] - 1 - math.floor(math.fmod(_elapsed * ANIMATION_SPEEDS[_current_cycle], SPRITE_COUNTS[_current_cycle]))
   end

   if (sprite_index ~= _sprite_index) then
      _sprite_index = sprite_index
      updateSpriteRect(
         0,
         _sprite_index * SPRITE_WIDTH,
         (_current_cycle - 1) * SPRITE_HEIGHT,
         SPRITE_WIDTH,
         SPRITE_HEIGHT
      ) -- id, x, y, width, height
   end
end

------------------------------------------------------------------------------------------------------------------------
function writeProperty(key, value)
   if (key == "show") then
      show()
   elseif (key == "hide") then
      hide()
   end
end

------------------------------------------------------------------------------------------------------------------------
function retrieveProperties()
   updateProperties(properties)
end
