
#ifndef AUDIOUPDATEDATA_H
#define AUDIOUPDATEDATA_H

#include "game/audio/audiorange.h"
#include "game/constants.h"

#include <cstdint>
#include <vector>

struct AudioUpdateData
{
   AudioUpdateBehavior _update_behavior{AudioUpdateBehavior::RangeBased};
   std::optional<AudioRange> _range;
   std::vector<int32_t> _room_ids;
   float _volume{0.0f};  // can differ from the reference volume
};

#endif  // AUDIOUPDATEDATA_H
