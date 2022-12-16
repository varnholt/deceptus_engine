#ifndef AUDIORANGE_H
#define AUDIORANGE_H

#include <optional>

struct AudioRange
{
   float _radius_far_px{0.0f};
   float _volume_far{0.0f};
   float _volume_close{1.0f};
};

#endif // AUDIORANGE_H
