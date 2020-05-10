v2d = require "data/scripts/enemies/vectorial2"


------------------------------------------------------------------------------------------------------------------------
function linearInterpolate(y1, y2, mu)
   return(y1 * (1.0 - mu) + y2 * mu)
end


------------------------------------------------------------------------------------------------------------------------
function cosineInterpolate(y1, y2, mu)
   mu2 = (1.0 - math.cos(mu * math.pi)) * 0.5
   val = (y1 * (1.0 - mu2) + y2 * mu2)
   -- print(string.format("y1: %f, y2: %f, mu: %f -> %f", y1, y2, mu, val))
   return val
end


------------------------------------------------------------------------------------------------------------------------
function cubicInterpolate(y0, y1, y2, y3, mu)
   mu2 = mu * mu
   a0 = y3 - y2 - y0 + y1
   a1 = y0 - y1 - a0
   a2 = y2 - y0
   a3 = y1

   return (a0 * mu * mu2 + a1 * mu2 + a2 * mu + a3)
end


------------------------------------------------------------------------------------------------------------------------
function tableLength(T)
   local count = 0
   for _ in pairs(T) do count = count + 1 end
   return count
end


------------------------------------------------------------------------------------------------------------------------
function findIndex(track, time)
   size = 0
   for k, v in pairs(track) do
      if (v.time > time) then
         return k - 1
      end
      size = size + 1
   end
   -- reached end of table
   return size
end


------------------------------------------------------------------------------------------------------------------------
function getValueLinear(track, time)

   y1i = findIndex(track, time)
   size = tableLength(track)

   -- clamp
   if (y1i < 1) then
      p = v2d.Vector2D(track[1].x, track[1].y)
      return p
   elseif (y1i >= size) then
      p = v2d.Vector2D(track[size].x, track[size].y)
      return p
   else
      y2i = y1i + 1
   end

   -- do linear interpolation
   y1 = track[y1i]
   y2 = track[y2i]

   range = y2.time - y1.time
   mu = (time - y1.time) / range

   x = linearInterpolate(y1.x, y2.x, mu)
   y = linearInterpolate(y1.y, y2.y, mu)
   p = v2d.Vector2D(x, y)

   return p
end


------------------------------------------------------------------------------------------------------------------------
function getValueCos(track, time)

   y1i = findIndex(track, time)
   size = tableLength(track)

   -- clamp
   if (y1i < 1) then
      p = v2d.Vector2D(track[1].x, track[1].y)
      return p
   elseif (y1i >= size) then
      p = v2d.Vector2D(track[size].x, track[size].y)
      return p
   else
      y2i = y1i + 1
   end

   -- do cosine interpolation
   y1 = track[y1i]
   y2 = track[y2i]

   range = y2.time - y1.time
   mu = (time - y1.time) / range

   x = cosineInterpolate(y1.x, y2.x, mu)
   y = cosineInterpolate(y1.y, y2.y, mu)
   p = v2d.Vector2D(x, y)

   return p
end


------------------------------------------------------------------------------------------------------------------------
function getValueCubic(track, time)

   y1i = findIndex(track, time)
   y0i = y1i - 1
   y2i = y1i + 1
   y3i = y1i + 2

   size = tableLength(track)

   y0i = math.max(1, y0i)
   y1i = math.max(1, y1i)
   y2i = math.max(1, y2i)
   y3i = math.max(1, y3i)

   y0i = math.min(size, y0i)
   y1i = math.min(size, y1i)
   y2i = math.min(size, y2i)
   y3i = math.min(size, y3i)

   y0 = track[y0i]
   y1 = track[y1i]
   y2 = track[y2i]
   y3 = track[y3i]

   range = y2.time - y1.time

   if (range < 0.01) then
      return v2d.Vector2D(y1.x, y1.y)
   end

   mu = (time - y1.time) / range
   x = cubicInterpolate(y0.x, y1.x, y2.x, y3.x, mu)
   y = cubicInterpolate(y0.y, y1.y, y2.y, y3.y, mu)
   p = v2d.Vector2D(x, y)

   return p
end

