#include "tmximage.h"


void TmxImage::deserialize(tinyxml2::XMLElement *e)
{
   mSource = e->Attribute("source");
   mWidth  = e->IntAttribute("width");
   mHeight = e->IntAttribute("height");

//   printf("   image (source: %s, width: %d, height: %d)\n",
//      mSource.c_str(),
//      mWidth,
//      mHeight
//   );
}

