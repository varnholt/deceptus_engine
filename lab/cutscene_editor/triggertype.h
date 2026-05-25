#pragma once

#include <cstdint>

/// \brief selects how a cutscene entry is dispatched.
enum class TriggerType : int32_t
{
   At = 0,   //!< fires once when elapsed time reaches _at_time
   On = 1,   //!< fires when a named event is notified
};
