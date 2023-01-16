#include "tmxtileset.h"

#include "framework/tools/log.h"
#include "tmximage.h"
#include "tmxtile.h"

#include <iostream>


TmxTileSet::TmxTileSet()
{
   _type = TmxElement::Type::TypeTileSet;
}


void TmxTileSet::parseTileSet(tinyxml2::XMLElement* element, const std::shared_ptr<TmxParseData>& parse_data)
{
   TmxElement::deserialize(element, parse_data);

   _tile_width_px  = element->IntAttribute("tilewidth");
   _tile_height_px = element->IntAttribute("tileheight");
   _tile_count  = element->IntAttribute("tilecount");
   _columns    = element->IntAttribute("columns");
   _rows       = _columns > 0 ? (_tile_count / _columns) : 0;

   auto node = element->FirstChild();
   while (node)
   {
      auto child_element = node->ToElement();

      if (!child_element)
      {
         node = node->NextSibling();
         continue;
      }

      std::shared_ptr<TmxElement> tmp;
      std::shared_ptr<TmxTile> tile;

      if (child_element->Name() == std::string("image"))
      {
         _image = std::make_shared<TmxImage>();
         tmp = _image;
      }
      else if (child_element->Name() == std::string("tile"))
      {
         tile = std::make_shared<TmxTile>();
         tmp = tile;
      }

      // deserialize detected elements
      if (tmp)
      {
         tmp->deserialize(child_element, parse_data);
      }
      else
      {
         Log::Error() << child_element->Name() << " is not supported for TmxTileSet";
      }

      if (tile)
      {
         _tile_map[tile->_id] = tile;
      }

      node = node->NextSibling();
   }
}


void TmxTileSet::deserialize(tinyxml2::XMLElement *element, const std::shared_ptr<TmxParseData>& parse_data)
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
         parseTileSet(docElem, parse_data);
      }
      else
      {
        Log::Error() << "TmxTileSet::deserialize: source not found: " << filename;
        exit(-1);
      }
   }
   else
   {
      parseTileSet(element, parse_data);
   }
}

