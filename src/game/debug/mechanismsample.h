#pragma once

#ifdef DEVELOPMENT_MODE

#include <cstdint>
#include <string>

struct MechanismSample
{
   std::string name;       //!< mechanism type name from objectName()
   float update_ms{0.0f};  //!< total update cost of every instance of this type, for the sampled frame
   float draw_ms{0.0f};    //!< total draw cost of every instance of this type, for the sampled frame
   int32_t count{0};       //!< instances of this type that were updated in that frame

   //!< These are totals rather than per-instance averages on purpose. The average said a SmokeEffect
   //!< costs 0.061 ms and left it at that, which cannot be ranked against anything - fifteen cheap
   //!< instances outweigh one expensive one, and the frame pays the total. The count is kept so the
   //!< per-instance figure is still derivable, and so "why is this type expensive" separates into
   //!< "each one is slow" and "there are a lot of them".
};

#endif  // DEVELOPMENT_MODE
