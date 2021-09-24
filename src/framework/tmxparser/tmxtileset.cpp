#include "tmxtileset.h"

#include "tmximage.h"
#include "tmxtile.h"
#include <iostream>


TmxTileSet::TmxTileSet()
{
   _type = TmxElement::TypeTileSet;
}


TmxTileSet::~TmxTileSet()
{
   delete _image;
}


void TmxTileSet::parseTileSet(tinyxml2::XMLElement* element)
{
   TmxElement::deserialize(element);

   _tile_width_px  = element->IntAttribute("tilewidth");
   _tile_height_px = element->IntAttribute("tileheight");
   _tile_count  = element->IntAttribute("tilecount");
   _columns    = element->IntAttribute("columns");
   _rows       = _columns > 0 ? (_tile_count / _columns) : 0;

   //   printf(
   //      "tileset (name: %s, width: %d, height: %d, count: %d, cols: %d, rows: %d)\n",
   //      mName.c_str(),
   //      mTileWidth,
   //      mTileHeight,
   //      mTileCount,
   //      mColumns,
   //      mRows
   //   );

   tinyxml2::XMLNode* node = element->FirstChild();
   while(node != nullptr)
   {
      tinyxml2::XMLElement* childElement = node->ToElement();
      if (childElement != nullptr)
      {
         TmxElement* tmp = nullptr;
         TmxTile* tile = nullptr;

         if (childElement->Name() == std::string("image"))
         {
            _image = new TmxImage();
            tmp = _image;
         }
         else if (childElement->Name() == std::string("tile"))
         {
            tile = new TmxTile();
            tmp = tile;
         }

         // deserialize detected elements
         if (tmp != nullptr)
         {
            tmp->deserialize(childElement);
         }
         else
         {
            printf(
               "%s is not supported for TmxTileSet\n",
               childElement->Name()
            );
         }

         if (tile != nullptr)
         {
            _tile_map[tile->_id] = tile;
         }
      }

      node = node->NextSibling();
   }
}


void TmxTileSet::deserialize(tinyxml2::XMLElement *element)
{
   _first_gid   = element->IntAttribute("firstgid");
   _source      = element->Attribute("source") ? element->Attribute("source") : "";

   // id is read later because source can be an external file
   if (!_source.empty())
   {
      tinyxml2::XMLDocument doc;

      std::string filename = _path.append(_source).string();
      if (doc.LoadFile(filename.c_str()) == tinyxml2::XML_SUCCESS)
      {
         tinyxml2::XMLElement* docElem = doc.FirstChildElement();
         parseTileSet(docElem);
      }
      else
      {
        std::cout << "TmxTileSet::deserialize: source not found: " << filename << std::endl;
        exit(-1);
      }
   }
   else
   {
      parseTileSet(element);
   }
}

