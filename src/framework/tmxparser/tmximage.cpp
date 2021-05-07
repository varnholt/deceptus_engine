#include "tmximage.h"


void TmxImage::deserialize(tinyxml2::XMLElement *e)
{
   _source = e->Attribute("source");
   _width_px  = e->IntAttribute("width");
   _height_px = e->IntAttribute("height");
}

