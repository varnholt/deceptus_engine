--[[
   vectorial2.lua ver 0.2 - A library for 2D vectors.
   Copyright (C) 2015 Leo Tindall

   ---
    All operators work as expected except modulo (%) which is vector distance and concat (..) which is linear distance.
   ---

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
]]

--[[
   Local modification: the methods and metamethods below used to be defined *inside* Vector2D, so
   every constructed vector built its own copy of all of them. That came to 3 tables plus 24 closures
   per vector - 27 allocations to store two numbers.

   BEFORE - every Vector2D(x, y) call built this whole graph:

       vector ---> { v ---> {x, y}                     3 tables
                     deepcopy   ---> closure           + 24 fresh closures, one per method
                     getAngle   ---> closure             and per operator, every single call
                     getLength  ---> closure
                     setX/setY/getX/getY -> 4 closures
                     sub/add/mul/len/norm -> 5 closures
                     ...                              }
                  metatable ---> { __add ---> closure    <- also rebuilt per vector
                                   __sub ---> closure
                                   __eq/__lt/__le/__unm/__mul/__div/__mod/__concat/__tostring
                                                      }

   AFTER - the graph is built once when the module loads, and a vector is just its components:

       methods  (one table, module scope) <--.
       mt.__index --------------------------'
                                             \
       vector_a ---> { v ---> {x, y} } --------+---> mt        2 tables per vector
       vector_b ---> { v ---> {x, y} } --------'                (and only the components differ)

   27 allocations -> 2. Same for the operators: they went through deepcopy, which copied the whole
   graph including the metatable; now the metatable is carried over by reference, which is the same
   metamethods either way.

   It mattered because of how often that happens. Every enemy gets movedTo(x, y) and
   playerMovedTo(x, y) once per simulation step, and nearly every script implements those as
   "_position_px = v2d.Vector2D(x, y)". Profiled on the catacombs with 24 enemies in range, those two
   one-line callbacks cost 0.165 ms per frame against 0.143 ms for every enemy's entire AI update -
   4.4 us to store two numbers.

   They are shared now, created once when the module loads, so a vector is the table plus its v
   table. Every method body and every operator is unchanged; what moved is where they live. deepcopy
   also carries the metatable over by reference rather than deep copying it, which is the same
   metamethods either way and saves the copy.
]]
local module = {}

--Shared by every vector. Instances hold only their components and reach these through __index.
local methods = {}
local mt = {} --Metatable
mt.__index = methods

      module.Vector2D = function (ix, iy)
   local v2d = {}
         v2d.v = {}
         v2d.v.x = ix
         v2d.v.y = iy

   return setmetatable(v2d, mt)
end

   function methods:deepcopy(orig) --Deeply copy a table. This is for the operation metatables.
         local orig_type = type(orig)
         local copy
         if orig_type == 'table' then
            copy = {}
            for orig_key, orig_value in next, orig, nil do
                     copy[self:deepcopy(orig_key)] = self:deepcopy(orig_value)
            end
            --the metatable is shared rather than owned, so it is carried over as it is. deep copying
            --it produced a distinct table holding the same metamethods, which behaves identically
            setmetatable(copy, getmetatable(orig))
         else -- number, string, boolean, etc
            copy = orig
         end
         return copy
   end

   --Vector Specific Math

   function methods:getAngle() --Return the 2D angle of the vector IN RADIANS!.
      return math.atan2(self:getY(), self:getX())
   end

   function methods:getLength() --Return the length of the vector (i.e. the distance from (0,0), see README.md for examples of using this)
      local origin = self:deepcopy(self) --Get a new vector to work with
      origin:setX(0) --Set the origin equal to the geometric origin
      origin:setY(0)
      return self .. origin --Linear distance from us to the origin
   end

   function module.average (vectors)
      local n = #vectors
      local tmp = module.Vector2D(0, 0)
      local j = 1 --Position in new_vectors
      if n == 0 then
         error("average() called with 0 inputs!")
      end
      for i, vector in ipairs(vectors) do
         tmp = tmp + vector
      end
      return tmp / module.Vector2D(n, n)
   end


   --Comparisons

   mt.__eq = function(lhs, rhs)
      --Equal To operator for vector2Ds
      return (lhs.getX() == rhs.getX()) and (lhs.getY() == rhs.getY())
   end

   mt.__lt = function(lhs, rhs)
      --Less Than operator for vector2Ds
      return (sqrt((lhs.getX()^2) + (lhs.getY()^2)) < sqrt((rhs.getX()^2) + (rhs.getY()^2))) --We do this to compute the linear value of the vector so that, for example, (a % b) < (c % d) will not be broken.
   end

   mt.__le = function(lhs, rhs)
      --Less Than Or Equal To operator for vector2Ds
      return (sqrt((lhs.getX()^2) + (lhs.getY()^2)) <= sqrt((rhs.getX()^2) + (rhs.getY()^2))) --We do this to compute the linear value of the vector so that, for example, (a % b) < (c % d) will not be broken.
   end

   --Operations

   function methods:setX(x)
      self.v.x = x
   end

   function methods:setY(y)
      self.v.y = y
   end

   function methods:getX()
      return self.v.x
   end

   function methods:getY()
      return self.v.y
   end

   mt.__unm = function(rhs)
      --Unary Minus (negation) operator for Vector2Ds
      local out = rhs:deepcopy(rhs) --Copy the operand for the output (else the output won't have metamethods)
      out:setX(-rhs:getX()) --Operate on the X property
      out:setY(-rhs:getY()) --Operate on the Y property
      return out
   end

   mt.__add = function(lhs, rhs)
      --Addition operator for Vector2Ds
      local out = lhs:deepcopy(lhs)--Copy the operand for the output (else the output won't have metamethods)
      out:setX(lhs:getX() + rhs:getX()) --Operate on the X property
      out:setY(lhs:getY() + rhs:getY()) --Operate on the Y property
      return out
   end

   mt.__sub = function(lhs, rhs)
      --Subtraction operator for Vector2Ds
      local out = lhs:deepcopy(lhs)--Copy the operand for the output (else the output won't have metamethods)
      out:setX(lhs:getX() - rhs:getX()) --Operate on the X property
      out:setY(lhs:getY() - rhs:getY()) --Operate on the Y property
      return out
   end

   mt.__mul = function(lhs, rhs)
      --Multiplication operator for Vector2Ds
      local out = lhs:deepcopy(lhs)--Copy the operand for the output (else the output won't have metamethods)
      out:setX(lhs:getX() * rhs:getX()) --Operate on the X property
      out:setY(lhs:getY() * rhs:getY()) --Operate on the Y property
      return out
   end

   mt.__div = function(lhs, rhs)
      --Division operator for Vector2Ds
      local out = lhs:deepcopy(lhs)--Copy the operand for the output (else the output won't have metamethods)
      out:setX(lhs:getX() / rhs:getX()) --Operate on the X property
      out:setY(lhs:getY() / rhs:getY()) --Operate on the Y property
      return out
   end

   mt.__mod = function(lhs, rhs)
      --Vector distance operator for Vector2Ds. Denoted by modulo (%)
      local out = lhs:deepcopy(lhs)		--Copy the operand for the output (else the output won't have metamethods)
      out:setX(math.abs(rhs:getX() - lhs:getX())) --Operate on the X property
      out:setY(math.abs(rhs:getY() - lhs:getY())) --Operate on the Y property
      return out
   end

   mt.__concat = function(lhs, rhs)
      --Linear distance operator for Vector2Ds. Denoted by concat (..)
      local out = 0		--This is a linear operation, so no deepcopy.
      out = math.sqrt(((lhs:getX() - rhs:getX())^2) + ((rhs:getY() - lhs:getY())^2)) --Distance formula
      return out
   end

   function methods:sub(other)
      -- Subtract another vector from this vector
      return self - other
   end

   function methods:add(other)
      -- Add another vector to this vector
      return self + other
   end

   function methods:mul(scalar)
      -- Multiply this vector by a scalar
      if type(scalar) == "number" then
         -- If scalar is a number, multiply both components by it
         local result = self:deepcopy(self)
         result:setX(self:getX() * scalar)
         result:setY(self:getY() * scalar)
         return result
      else
         -- If scalar is another vector, use component-wise multiplication
         return self * scalar
      end
   end

   function methods:len()
      -- Get the length (magnitude) of the vector
      return math.sqrt((self:getX() * self:getX()) + (self:getY() * self:getY()))
   end

   function methods:norm()
      -- Normalize the vector (get unit vector in same direction)
      local length = self:len()
      if length == 0 then
         -- Return zero vector if length is zero to avoid division by zero
         return module.Vector2D(0, 0)
      else
         return module.Vector2D(self:getX() / length, self:getY() / length)
      end
   end

   mt.__tostring = function(self)
      --tostring handler for Vector2D
      local out = ""	--This is a string operation, so no deepcopy.
      out = "[(X:"..self:getX().."),(Y:"..self:getY()..")]"
      return out
   end

return module
