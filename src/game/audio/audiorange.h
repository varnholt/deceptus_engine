#ifndef AUDIORANGE_H
#define AUDIORANGE_H

#include <optional>

/// \brief distance-based volume profile with near and far radii in pixels and their target volume levels.
struct AudioRange
{
   float _radius_far_px{0.0f};
   float _volume_far{0.0f};

   float _radius_near_px{0.0f};
   float _volume_near{1.0f};
};

#endif  // AUDIORANGE_H
