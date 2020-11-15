#include "tmximage.h"


void TmxImage::deserialize(tinyxml2::XMLElement *e)
{
   mSource = e->Attribute("source");
   mWidth  = e->IntAttribute("width");
   mHeight = e->IntAttribute("height");
}

