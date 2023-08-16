Key = {
   KeyUp    = 0x01,
   KeyDown  = 0x02,
   KeyLeft  = 0x04,
   KeyRight = 0x08,
   KeyFire  = 0x10,
   KeyRun   = 0x20
}


Alignment = {
   AlignmentUp    = 0x01,
   AlignmentDown  = 0x02,
   AlignmentLeft  = 0x04,
   AlignmentRight = 0x08
}


WeaponType = {
   Invalid = 0,
   Bow     = 1,
   Gun     = 2,
   Sowrd   = 3
}

AudioUpdateBehavior = {
   AlwaysOn = 0,
   RangeBased = 1,
   RoomBased = 2
}

------------------------------------------------------------------------------------------------------------------------
function alignmentFromString(value)
   if (value == "right") then
      return Alignment["AlignmentRight"]
   elseif (value == "left") then
      return Alignment["AlignmentLeft"]
   elseif (value == "up") then
      return Alignment["AlignmentUp"]
   elseif (value == "down") then
      return Alignment["AlignmentDown"]
   end
end

------------------------------------------------------------------------------------------------------------------------
function audioUpdateBehaviorFromString(value)
   if (value == "range_based") then
      return AudioUpdateBehavior["RangeBased"]
   elseif (value == "room_based") then
      return AudioUpdateBehavior["RoomBased"]
   end

   return AudioUpdateBehavior["AlwaysOn"]
end

