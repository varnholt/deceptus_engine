-- row 0 idle l
-- row 1 idle r
-- row 2 idle-blink l
-- row 3 idle-blink r
-- row 4 prepare for throw l
-- row 5 prepare for throw r
-- row 6 throw bomb l
-- row 7 throw bomb r
-- row 8 reload bomb l
-- row 9 reload bomb r
--
-- row 10 bomb tick warning l (48x48)
-- row 11 bomb tick warning r (48x48)
-- row 12 bomb (48x48)
-- row 13 bomb (48x48)
--
-- sprite is 4 x 24, 2 x 24
--
--  +---+---+---+---+
--  |  /|///|///|/  |
--  +---+---+---+---+
--  |  /|///|///|/  |
--  +---+---+---+---+
--
-- origin is in the bottom center
--
-- 8 sprites per row
--
--
-- bomb is 2 x 24, 2 x 24
-- 16 per row
--
-- origin is in the vertical/horizontal center
-- +---+---+
-- | ##|## |
-- +---+---+
-- | ##|## |
-- +---+---+


------------------------------------------------------------------------------------------------------------------------
require "data/scripts/enemies/constants"
v2d = require "data/scripts/enemies/vectorial2"


------------------------------------------------------------------------------------------------------------------------
properties = {
   sprite = "data/sprites/enemy_minik_bomber.png",
   damage = 0
}


------------------------------------------------------------------------------------------------------------------------
SPRITE_WIDTH = 4 * 24
SPRITE_HEIGHT = 2 * 24

CYCLE_IDLE = 0
CYCLE_IDLE_BLINK = 1
CYCLE_PREPARE_THROW = 2
CYCLE_THROW = 3
CYCLE_RELOAD = 4

-- should rather have a class instead :/
--
--                   i  i  p  t  r
CYCLE_LENGTHS =     {8, 8, 4, 4, 8}
CYCLE_ROWS_LEFT =   {0, 2, 4, 4, 6}
CYCLE_ROWS_RIGHT =  {1, 3, 5, 5, 7}
CYCLE_START_INDEX = {0, 0, 0, 4, 0}
CYCLE_LOOPED = {true, true, false, false, false}
ANIMATION_SPEEDS = {10.0, 10.0, 10.0, 10.0, 10.0}

THROW_DISTANCE_PX = 300


------------------------------------------------------------------------------------------------------------------------
_ready_to_throw = true
_pos = v2d.Vector2D(0, 0)
_pos_player = v2d.Vector2D(0, 0)

_sprite_index = 0

_elapsed_s = 0.0
_elapsed_current_sprite_s = 0.0

_projectile_index = 0

_throw_dir_x = -1.0
_alignment_offset = 0
_points_to_left = true

_current_cycle = CYCLE_IDLE
_can_throw = false


------------------------------------------------------------------------------------------------------------------------
function initialize()
   addShapeCircle(0.32, 0.48, 0.24) -- drag that box2d circle to where we want it to be
   setSpriteOffset(0, 21, 4); -- now align that circle with our texture

   -- set up boom and audio distance
   addHitbox(0, 0, 48, 24)
   -- addAudioRange(400.0, 0.0, 200.0, 1.0)
   -- addSample("mechanism_cannon_1.wav")
   -- addSample("mechanism_cannon_2.wav")
   -- addSample("mechanism_cannon_3.wav")
   -- addSample("mechanism_cannon_4.wav")
   -- addSample("mechanism_cannon_boom_1.wav")
   -- addSample("mechanism_cannon_boom_2.wav")

   addWeapon(WeaponType["Gun"], 1000, 60, 0.5, 0.2) -- interval, damage, gravity_scale, radius

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

   --    registerHitSamples(
   --       "data/sprites/enemy_pirate_cannon_cannonball.png",
   --       "mechanism_cannon_boom_1.wav",
   --       0.5,
   --       "mechanism_cannon_boom_2.wav",
   --       0.5
   --    )

   -- https://github.com/varnholt/deceptus_engine/tree/master/doc/lua_interface#updateprojectileanimation
   updateProjectileAnimation(
      0,
      "data/sprites/enemy_minik_bomber.png",
      2 * 24,
      2 * 24,
      (2 * 24) / 2,
      (2 * 24) / 2,
      0.05,
      8,
      16,
      9 * 16 -- start frame is in row 8/9
   )

end


------------------------------------------------------------------------------------------------------------------------
function writeProperty(key, value)
   if (key == "alignment") then
      if (value == "right") then
         -- print("setting alignment to left")
         _points_to_left = false
         _throw_dir_x = 1.0
         _alignment_offset = 5 * SPRITE_HEIGHT
      end
   end
end


------------------------------------------------------------------------------------------------------------------------
function timeout(id)
end


------------------------------------------------------------------------------------------------------------------------
function fire()
   -- playSample(string.format("mechanism_cannon_%d.wav", math.random(1, 4)), 0.5)

   useGun(
      0,
      _pos:getX() + _throw_dir_x * (_points_to_left and 32 or 64),
      _pos:getY(),
      _throw_dir_x * 2.5,
      -3.0
   );
end


------------------------------------------------------------------------------------------------------------------------
function retrieveProperties()
   updateProperties(properties)
end


------------------------------------------------------------------------------------------------------------------------
function movedTo(x, y)
   _pos = v2d.Vector2D(x, y)
end


------------------------------------------------------------------------------------------------------------------------
function playerMovedTo(x, y)
   _pos_player = v2d.Vector2D(x, y)
end


------------------------------------------------------------------------------------------------------------------------
function decide(dt)

   cycle_complete = math.floor(_elapsed_current_sprite_s) > getMaxCycle() - 1

   -- it's time for a new cycle
   if (cycle_complete) then

      next_cycle = _current_cycle

      if (_can_throw and (_current_cycle == CYCLE_IDLE or _current_cycle == CYCLE_IDLE_BLINK)) then
         next_cycle = CYCLE_PREPARE_THROW
      end

      if (_current_cycle == CYCLE_PREPARE_THROW) then
         next_cycle = CYCLE_THROW
      end

      if (_current_cycle == CYCLE_THROW) then
         next_cycle = CYCLE_RELOAD
      end

      if (_current_cycle == CYCLE_RELOAD) then
         next_cycle = CYCLE_IDLE
      end

      if (next_cycle ~= _current_cycle) then
         _current_cycle = next_cycle
         _elapsed_current_sprite_s = 0.0
      end

   end

end


------------------------------------------------------------------------------------------------------------------------
function getCurrentCycle(dt)
   _elapsed_current_sprite_s = _elapsed_current_sprite_s + dt * ANIMATION_SPEEDS[_current_cycle + 1]
   looped = CYCLE_LOOPED[_current_cycle + 1]

   cycle = 0
   if (looped) then
      return math.fmod(_elapsed_current_sprite_s, getMaxCycle())
   end

   return math.min(_elapsed_current_sprite_s, getMaxCycle() - 1)
end


------------------------------------------------------------------------------------------------------------------------
function updateSprite(dt)

   cycle = getCurrentCycle(dt)

   sprite_index = math.floor(cycle)

   if (_current_sprite ~= sprite_index) then
      _current_sprite = sprite_index

      updateSpriteRect(
         0,
         sprite_index * SPRITE_WIDTH + CYCLE_START_INDEX[_current_cycle + 1] * SPRITE_WIDTH,
         getSpriteOffsetY() * SPRITE_HEIGHT,
         SPRITE_WIDTH,
         SPRITE_HEIGHT
      )
   end
end


------------------------------------------------------------------------------------------------------------------------
function getSpriteOffsetY()
   return _points_to_left and (CYCLE_ROWS_LEFT[_current_cycle + 1]) or (CYCLE_ROWS_RIGHT[_current_cycle + 1])
end


------------------------------------------------------------------------------------------------------------------------
function getMaxCycle()
   return CYCLE_LENGTHS[_current_cycle + 1]
end



------------------------------------------------------------------------------------------------------------------------
function update(dt)
   _elapsed_s = _elapsed_s + dt
   updateThrowCondition(dt)
   decide(dt)
   updateSprite(dt)
end


------------------------------------------------------------------------------------------------------------------------
function setPath(name, table)
end


------------------------------------------------------------------------------------------------------------------------
function updateThrowCondition(dt)

   if (_current_cycle == CYCLE_IDLE or _current_cycle == CYCLE_IDLE_BLINK) then
      _can_throw = false

      if (math.abs(_pos:getY() - _pos_player:getY()) < 24) then
         if (math.abs(_pos:getX() - _pos_player:getX()) < THROW_DISTANCE_PX) then

            player_is_left = (_pos:getX() > _pos_player:getX())

            within_throw_distance = false
            if (player_is_left and _points_to_left) then
               within_throw_distance = true
            elseif (not player_is_left and not _points_to_left) then
               within_throw_distance = true
            end

            -- print(
            --    string.format("pos player: %f %f, pos self: %f, %f",
            --       _pos_player:getX(),
            --       _pos_player:getY(),
            --       _pos:getX(),
            --       _pos:getY()
            --    )
            -- )

            if (within_throw_distance) then
               _can_throw = isPhsyicsPathClear(
                     _pos:getX(),
                     _pos:getY(),
                     _pos_player:getX(),
                     _pos_player:getY()
                  )
            end
         end
      end
   elseif (_current_cycle == CYCLE_THROW) then

      -- update projectile index
      cycle = getCurrentCycle(dt)
      if (cycle == 3) then
         fire()
      end
   end
end

