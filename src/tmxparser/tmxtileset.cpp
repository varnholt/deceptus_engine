#include "tmxtileset.h"

#include "tmximage.h"
#include "tmxtile.h"
#include <iostream>


TmxTileSet::TmxTileSet()
{
   mType = TmxElement::TypeTileSet;
}


void TmxTileSet::parseTileSet(tinyxml2::XMLElement* element)
{
   TmxElement::deserialize(element);

   mTileWidth  = element->IntAttribute("tilewidth");
   mTileHeight = element->IntAttribute("tileheight");
   mTileCount  = element->IntAttribute("tilecount");
   mColumns    = element->IntAttribute("columns");
   mRows       = mColumns > 0 ? (mTileCount / mColumns) : 0;

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
            mImage = new TmxImage();
            tmp = mImage;
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
            mTileMap[tile->mId] = tile;
         }
      }

      node = node->NextSibling();
   }
}


void TmxTileSet::deserialize(tinyxml2::XMLElement *element)
{
   mFirstGid   = element->IntAttribute("firstgid");
   mSource     = element->Attribute("source") ? element->Attribute("source") : "";

   // id is read later because source can be an external file
   if (!mSource.empty())
   {
      tinyxml2::XMLDocument doc;

      std::string filename = mPath.append(mSource).string();
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

