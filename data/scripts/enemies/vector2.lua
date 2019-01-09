local META = {}
META = 
{
	__index = function(tbl, key) return META[key] end,
	__unm = function(lhs) return vec2(-lhs.x, lhs.y) end,
	__add = function(lhs, rhs) return vec2(lhs.x + (rhs.x or rhs), lhs.y + (rhs.y or rhs)) end,
	__sub = function(lhs, rhs) return vec2(lhs.x - (rhs.x or rhs), lhs.y - (rhs.y or rhs)) end,
	__mul = function(lhs, rhs) return vec2(lhs.x * (rhs.x or rhs), lhs.y * (rhs.y or rhs)) end,
	__div = function(lhs, rhs) return vec2(lhs.x / (rhs.x or rhs), lhs.y / (rhs.y or rhs)) end,
	__mod = function(lhs, rhs) return vec2(lhs.x % (rhs.x or rhs), lhs.y % (rhs.y or rhs)) end,
	__pow = function(lhs, rhs) return vec2(lhs.x ^ (rhs.x or rhs), lhs.y ^ (rhs.y or rhs)) end,
	__tostring = function(lhs) return string.format("vec2(%s,%s)",lhs.x,lhs.y) end,
	__le = function(lhs, rhs) return lhs.x <= rhs.x and lhs.y <= rhs.y end,
	__lt = function(lhs, rhs) return lhs.x < rhs.x and lhs.y < rhs.y end,
	__eq = function(lhs, rhs) return lhs.x == rhs.x and lhs.y == rhs.y end,	
	
	clamp = function(lhs, min, max) return vec2(math.min(math.max(lhs.x, min.x), max.x), math.min(math.max(lhs.y, min.y), max.y)) end,
	length = function(lhs) return math.sqrt(lhs.x*lhs.x+lhs.y*lhs.y) end,
	distance = function(lhs, vec) return math.abs(math.sqrt((lhs.x-vec.x)^2+(lhs.y-vec.y)^2)) end,
	dot_product = function(lhs, vec) return lhs.x*vec.x+lhs.y*vec.y end,	
	angle_atan = function(lhs, vec)	return math.deg(math.atan2( vec:length(), lhs:length())) end,
	angle_cos = function(lhs, vec)	return math.deg(math.acos(lhs:dot_product(vec)/(lhs:length()*vec:length()))) end,
	normalize = function(lhs) return vec2(lhs.x/lhs:length(),lhs.y/lhs:length()) end,
	perpendecular = function(lhs, rhs, sign) return ((lhs+rhs)/2):rotate_around_axis(lhs, 90*math.min(math.max(sign, -1), 1)) end,
	
	rotate = function(lhs, ang) 
		
		ang = math.rad(ang)
		
		local c = math.cos(ang)
		local s = math.sin(ang)
		
		return vec2(lhs.x*c-lhs.y*s, lhs.x*s+lhs.y*c)
		 
	end,
	
	rotate_around_axis = function(lhs, ang, pos) 
		
		ang = math.rad(ang)
		
		local c = math.cos(ang)
		local s = math.sin(ang)
		
		return vec2((pos.x+lhs.x)*c-(pos.y+lhs.y)*s, (pos.x+lhs.x)*s+(pos.y+lhs.y)*c)

	end,
}

vec2 = function(x, y) return setmetatable({x = math.floor(x or 0, 4), y = math.floor(y or x or 0, 4)}, META) end

--[[
Uncomment for see results

print("Clamp vectors:", vec2(1, 0):clamp(vec2(0, 0), vec2(0.5, 0)))
print("Length vector:", vec2(1, 0):length())
print("Distance between two vectors:", vec2(0, 0):distance(vec2(1000, 0)))
print("Scalar product (Dot product):", vec2(15,25):dot_product())
print("Angle btween two vectors from ATAN (90):", vec2(1, 0):angle_atan(vec2(0, 1)))
print("Angle btween two vectors from COS (180):", vec2(1, 0):angle_cos(vec2(0, 1)))
print("Vector normalization:", vec2(0.99, 561):normalize())
print("Perpendecular from two points:", vec2(0, 1):perpendecular(vec2(1, 0)))
print("Rotate on 35 degrees:", vec2(1, 0):rotate(35))
print("Rotate on 90 degrees around axis:", vec2(1, 0):rotate_around_axis(90, vec(0, 1)))

]]--