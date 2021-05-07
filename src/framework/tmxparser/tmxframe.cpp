#include "tmxframe.h"

void TmxFrame::deserialize(tinyxml2::XMLElement *e)
{
   _tile_id = e->IntAttribute("tileid");
   _duration_ms = e->IntAttribute("duration");
}

