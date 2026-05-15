#pragma once

#ifdef DEVELOPMENT_MODE

#include <string>

struct MechanismSample
{
   std::string name;       //!< mechanism type name from objectName()
   float update_ms{0.0f};  //!< average update cost this frame in milliseconds
   float draw_ms{0.0f};    //!< average draw cost this frame in milliseconds
};

#endif  // DEVELOPMENT_MODE
