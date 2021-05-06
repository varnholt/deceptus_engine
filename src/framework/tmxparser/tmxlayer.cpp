#include "tmxlayer.h"

// tmxparser
#include "tmxchunk.h"
#include "tmxproperties.h"
#include "tmxtools.h"

#include <iostream>
#include <sstream>


TmxLayer::TmxLayer()
{
   mType = TmxElement::TypeLayer;
}


TmxLayer::~TmxLayer()
{
   delete[] mData;
}


void TmxLayer::deserialize(tinyxml2::XMLElement * element)
{
   TmxElement::deserialize(element);

   mWidth  = element->IntAttribute("width");
   mHeight = element->IntAttribute("height");
   mOpacity = element->FloatAttribute("opacity", 1.0f);
   mVisible = element->BoolAttribute("visible", true);

  //   printf(
  //      "layer: %s (width: %d, height: %d, opacity: %f)\n",
  //      mName.c_str(),
  //      mWidth,
  //      mHeight,
  //      mOpacity
  //   );

   tinyxml2::XMLNode* node = element->FirstChild();
   while (node != nullptr)
   {
      tinyxml2::XMLElement* subElement = node->ToElement();

      if (subElement != nullptr)
      {
         if (subElement->Name() == std::string("data"))
         {
            tinyxml2::XMLNode* dataNode = subElement->FirstChild();

            while (dataNode != nullptr)
            {
              tinyxml2::XMLElement* chunkElement = dataNode->ToElement();

              TmxElement* innerElement = nullptr;

              // process chunk data
              if (chunkElement != nullptr)
              {
                 if (chunkElement->Name() == std::string("chunk"))
                 {
                    auto chunk = new TmxChunk();
                    chunk->deserialize(chunkElement);
                    innerElement = chunk;
                    chunks.push_back(chunk);
                 }
              }

              // there are no chunks, the layer data is raw
              if (!innerElement && dataNode != nullptr)
              {
                 mData = new int32_t[mWidth * mHeight];
                 std::string data = subElement->FirstChild()->Value();

                 // parse csv data and store it in mData array
                 std::stringstream stream(data);
                 std::string line;
                 int y = 0;

                 while(std::getline(stream, line, '\n'))
                 {
                    TmxTools::trim(line);
                    if (line.empty())
                       continue;

                    int x = 0;
                    std::vector<std::string> rowContent = TmxTools::split(line, ',');
                    for (const std::string& valStr : rowContent)
                    {
                       int val = std::stoi(valStr);
                       mData[y * mWidth + x] = val;
                       x++;
                    }

                    y++;
                 }
              }

              dataNode = dataNode->NextSibling();
            }
         }
         else if (subElement->Name() == std::string("properties"))
         {
            mProperties = new TmxProperties();
            mProperties->deserialize(subElement);
         }
      }

      node = node->NextSibling();
   }

   if (!chunks.empty())
   {
     // after parsing all tmx chunks
     // - determine boundaries of all chunks
     // - create level with given dimensions
     // - merge chunks to layer
    int32_t xMin = chunks.at(0)->mX;
    int32_t xMax = chunks.at(0)->mX;
    int32_t yMin = chunks.at(0)->mY;
    int32_t yMax = chunks.at(0)->mY;

    for (auto i = 1u; i < chunks.size(); i++)
    {
      const auto c = chunks.at(i);

      if (c->mX < xMin)
      {
        xMin = c->mX;
      }

      if (c->mY < yMin)
      {
        yMin = c->mY;
      }

      if (c->mX > xMax)
      {
        xMax = c->mX;
      }

      if (c->mY > yMax)
      {
        yMax = c->mY;
      }
    }

    // the layer gets the smallest chunk offset
    mOffsetX = xMin;
    mOffsetY = yMin;

    // assume identical chunk sizes
    const auto chunkWidth  = chunks.at(0)->mWidth;
    const auto chunkHeight = chunks.at(0)->mHeight;

    mWidth  = (xMax - xMin) + chunkWidth;
    mHeight = (yMax - yMin) + chunkHeight;

    mData = new int32_t[mWidth * mHeight];

    // since we're dealing with patches of chunks there might be 'holes' in the map
    memset(mData, 0, mWidth * mHeight * sizeof (int32_t));

    // std::cout
    //   << "TmxLayer::deserialize: layer: " << mName << std::endl
    //   << "  dimensions: width: " << mWidth << "; height: " << mHeight << std::endl
    //   << "  offset: " << mOffsetX << "; " << mOffsetY << std::endl
    //   << "  x min: " << xMin << "; x max: " << xMax << std::endl
    //   << "  y min: " << yMin << "; y max: " << yMax << std::endl;

    for (const auto c : chunks)
    {
      for (auto y = 0; y < chunkHeight; y++)
      {
        for (auto x = 0; x < chunkWidth; x++)
        {
          // translate chunk coordinates to layer coordinates starting at 0; 0
          auto xl = c->mX + x -xMin;
          auto yl = c->mY + y -yMin;

          mData[yl * mWidth + xl] = c->mData[y * chunkWidth + x];
        }
      }
    }
  }
}

/*
 <layer name="physics" width="256" height="128" opacity="0.3">
  <data encoding="csv">
   <chunk x="0" y="-32" width="16" height="16">
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,7226,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,7226,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,7226,7226,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
  </chunk>
  <chunk x="16" y="-32" width="16" height="16">
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7226,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7226,
    7226,7226,7226,7226,7226,7226,7226,7226,7226,7226,7226,7226,7226,7226,7226,7226,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
  </chunk>
*/

/*
<?xml version="1.0" encoding="UTF-8"?>
<map version="1.0" tiledversion="1.1.5" orientation="orthogonal" renderorder="right-down" width="256" height="128" tilewidth="24" tileheight="24" infinite="1" nextobjectid="1">
 <tileset firstgid="1" source="crypts.tsx"/>
 <layer name="LEVEL" width="256" height="128">
  <data encoding="csv">
   <chunk x="-16" y="-144" width="16" height="16">
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,605,605,605,605,605,605,605,
0,0,0,0,0,0,0,0,0,605,605,605,605,605,605,605,
0,0,0,0,0,0,0,0,0,605,605,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,605,605,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,605,605,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,605,605,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,605,605,0,0,0,0,0
</chunk>
*/


    /*
      TmxLayer::deserialize: layer: LEVEL             TmxLayer::deserialize: layer: LEVEL
        dimensions: width: 288; height: 144             dimensions: width: 288; height: 160
        offset: 0; 0                                    offset: -16; -144
        x min: 0; x max: 272                            x min: -16; x max: 256
        y min: 0; y max: 128                            y min: -144; y max: 0
      TmxLayer::deserialize: layer: WATER             TmxLayer::deserialize: layer: WATER
        dimensions: width: 272; height: 112             dimensions: width: 272; height: 112
        offset: 0; 32                                   offset: 0; -112
        x min: 0; x max: 256                            x min: 0; x max: 256
        y min: 32; y max: 128                           y min: -112; y max: -16
      TmxLayer::deserialize: layer: OTHERS            TmxLayer::deserialize: layer: OTHERS
        dimensions: width: 64; height: 32               dimensions: width: 64; height: 16
        offset: 32; 96                                  offset: 16; -32
        x min: 32; x max: 80                            x min: 16; x max: 64
        y min: 96; y max: 112                           y min: -32; y max: -32
      TmxLayer::deserialize: layer: physics           TmxLayer::deserialize: layer: physics
        dimensions: width: 192; height: 32              dimensions: width: 208; height: 32
        offset: 16; 96                                  offset: 0; -32
        x min: 16; x max: 192                           x min: 0; x max: 192
        y min: 96; y max: 112                           y min: -32; y max: -16
   */
