-- easing functions for interpolating values over time.
-- all functions map a linear progress value in [0, 1] to a curved value in [0, 1].

local Easings = {
   -- constant speed throughout
   linear      = function(progress) return progress end,
   -- starts slow, accelerates — t² means early values stay small
   ease_in     = function(progress) return progress * progress end,
   -- starts fast, decelerates — mirror image of ease_in, written as 1 - (1-t)² expanded
   ease_out    = function(progress) return progress * (2.0 - progress) end,
   -- slow start, fast middle, slow end — two parabolas joined at progress=0.5,
   -- each scaled so the curve is continuous and reaches exactly 0.5 at the midpoint
   ease_in_out = function(progress)
      if progress < 0.5 then
         return 2.0 * progress * progress
      else
         return -1.0 + (4.0 - 2.0 * progress) * progress
      end
   end,
}

return Easings
