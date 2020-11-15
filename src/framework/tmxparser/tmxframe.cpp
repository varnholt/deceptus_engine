#include "tmxframe.h"

void TmxFrame::deserialize(tinyxml2::XMLElement *e)
{
   mTileId   = e->IntAttribute("tileid");
   mDuration = e->IntAttribute("duration");
}

