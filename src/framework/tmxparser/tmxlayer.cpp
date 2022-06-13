#include "tmxlayer.h"

// tmxparser
#include "tmxchunk.h"
#include "tmxproperties.h"
#include "tmxtools.h"

#include <iostream>
#include <sstream>


TmxLayer::TmxLayer()
{
   _type = TmxElement::TypeLayer;
}


TmxLayer::~TmxLayer()
{
   delete[] _data;
   delete _properties;
}


void TmxLayer::deserialize(tinyxml2::XMLElement* element, const std::shared_ptr<TmxParseData>& parse_data)
{
   TmxElement::deserialize(element, parse_data);

   _width_tl  = element->IntAttribute("width");
   _height_tl = element->IntAttribute("height");
   _opacity = element->FloatAttribute("opacity", 1.0f);
   _visible = element->BoolAttribute("visible", true);

   std::vector<TmxChunk*> chunks;

   auto node = element->FirstChild();
   while (node)
   {
      auto sub_element = node->ToElement();
      if (!sub_element)
      {
         node = node->NextSibling();
         continue;
      }

      if (sub_element->Name() == std::string("data"))
      {
         auto data_node = sub_element->FirstChild();
         while (data_node)
         {
           auto chunk_element = data_node->ToElement();

           TmxElement* inner_element = nullptr;

           // process chunk data
           if (chunk_element)
           {
              if (chunk_element->Name() == std::string("chunk"))
              {
                 auto chunk = new TmxChunk();
                 chunk->deserialize(chunk_element, parse_data);
                 inner_element = chunk;
                 chunks.push_back(chunk);
              }
           }

           // there are no chunks, the layer data is raw
           if (!inner_element && data_node != nullptr)
           {
              _data = new int32_t[_width_tl * _height_tl];
              std::string data = sub_element->FirstChild()->Value();

              // parse csv data and store it in mData array
              std::stringstream stream(data);
              std::string line;
              int32_t y = 0;

              while (std::getline(stream, line, '\n'))
              {
                 TmxTools::trim(line);
                 if (line.empty())
                    continue;

                 int32_t x = 0;
                 auto row_content = TmxTools::split(line, ',');
                 for (const std::string& val_str : row_content)
                 {
                    int val = std::stoi(val_str);
                    _data[y * _width_tl + x] = val;
                    x++;
                 }

                 y++;
              }
           }

           data_node = data_node->NextSibling();
         }
      }
      else if (sub_element->Name() == std::string("properties"))
      {
         _properties = new TmxProperties();
         _properties->deserialize(sub_element, parse_data);
      }

      node = node->NextSibling();
   }

   if (!chunks.empty())
   {
      // after parsing all tmx chunks
      // - determine boundaries of all chunks
      // - create level with given dimensions
      // - merge chunks to layer
      int32_t x_min = chunks.at(0)->_x_px;
      int32_t x_max = chunks.at(0)->_x_px;
      int32_t y_min = chunks.at(0)->_y_px;
      int32_t y_max = chunks.at(0)->_y_px;

      for (auto i = 1u; i < chunks.size(); i++)
      {
         const auto c = chunks.at(i);

         if (c->_x_px < x_min)
         {
            x_min = c->_x_px;
         }

         if (c->_y_px < y_min)
         {
            y_min = c->_y_px;
         }

         if (c->_x_px > x_max)
         {
            x_max = c->_x_px;
         }

         if (c->_y_px > y_max)
         {
            y_max = c->_y_px;
         }
      }

      // the layer gets the smallest chunk offset
      _offset_x_px = x_min;
      _offset_y_px = y_min;

      // assume identical chunk sizes
      const auto chunk_width  = chunks.at(0)->_width_px;
      const auto chunk_height = chunks.at(0)->_height_px;

      _width_tl  = (x_max - x_min) + chunk_width;
      _height_tl = (y_max - y_min) + chunk_height;

      _data = new int32_t[_width_tl * _height_tl];

      // since we're dealing with patches of chunks there might be 'holes' in the map
      memset(_data, 0, _width_tl * _height_tl * sizeof (int32_t));

      for (const auto c : chunks)
      {
         for (auto y = 0; y < chunk_height; y++)
         {
            for (auto x = 0; x < chunk_width; x++)
            {
               // translate chunk coordinates to layer coordinates starting at 0; 0
               auto xl = c->_x_px + x -x_min;
               auto yl = c->_y_px + y -y_min;

               _data[yl * _width_tl + xl] = c->_data[y * chunk_width + x];
            }
         }
      }
   }

   chunks.clear();
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
