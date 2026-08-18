#pragma once

#ifdef DEVELOPMENT_MODE

#include <string>

struct RenderSectionSample
{
   std::string name;         //!< render section as named in Level::draw
   float duration_ms{0.0f};  //!< cpu side cost of that section in the frame it was taken from
};

#endif  // DEVELOPMENT_MODE
