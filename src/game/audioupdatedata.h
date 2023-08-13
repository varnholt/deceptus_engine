
#ifndef AUDIOUPDATEDATA_H
#define AUDIOUPDATEDATA_H

#include "game/audiorange.h"
#include "game/constants.h"

struct AudioUpdateData
{
   AudioUpdateBehavior _update_behavior{AudioUpdateBehavior::RangeBased};
   std::optional<AudioRange> _range;
   std::optional<int32_t> _room_id;
   float _volume{0.0f};  // can differ from the reference volume
};

#endif  // AUDIOUPDATEDATA_H
