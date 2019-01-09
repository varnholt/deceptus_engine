------------------------------------------------------------------------------------------------------------------------
function getLength(l)
   len = 1 -- Lua table index is by default 1
   while list[len] do len = len + 1 end
   return len
end

------------------------------------------------------------------------------------------------------------------------
function applyFunction(f, l)
   i = 1
   while list[i] do
      f(list[i])
      i = i + 1
   end
end
