#include "tmxparser.h"

// tmxlayer
#include "tmximagelayer.h"
#include "tmxobjectgroup.h"
#include "tmxlayer.h"
#include "tmxtileset.h"
#include "tmxproperty.h"
#include "tmxproperties.h"

#include "tinyxml2/tinyxml2.h"

#include <iostream>


TmxParser::TmxParser()
{
}


void TmxParser::parse(const std::string &filename)
{
   auto z = 0;

   tinyxml2::XMLDocument doc;
   if (doc.LoadFile(filename.c_str()) == tinyxml2::XML_SUCCESS)
   {
      tinyxml2::XMLElement* docElem = doc.FirstChildElement();
      tinyxml2::XMLNode* n = docElem->FirstChild();
      while(n != nullptr)
      {
         tinyxml2::XMLElement* subElement = n->ToElement();
         if (subElement != nullptr)
         {
            TmxElement* element = nullptr;

            if (subElement->Name() == std::string("tileset"))
            {
               element = new TmxTileSet();
               dynamic_cast<TmxTileSet*>(element)->mPath = std::filesystem::path(filename).parent_path();
            }
            else if (subElement->Name() == std::string("layer"))
            {
               element = new TmxLayer();

               // that code does not belong here... should go to the level parser code!`
              // update z
               auto layer = dynamic_cast<TmxLayer*>(element);
               if (layer->mProperties)
               {
                  auto& map = layer->mProperties->mMap;
                  auto it = map.find("z");
                  if (it != map.end())
                  {
                    z = it->second->mValueInt;
                  }
               }
               dynamic_cast<TmxLayer*>(element)->mZ = z++;
            }
            else if (subElement->Name() == std::string("imagelayer"))
            {
               element = new TmxImageLayer();
               dynamic_cast<TmxImageLayer*>(element)->mZ = z++;
            }
            else if (subElement->Name() == std::string("objectgroup"))
            {
               element = new TmxObjectGroup();
               dynamic_cast<TmxObjectGroup*>(element)->mZ = z++;
            }

            if (element != nullptr)
            {
               element->deserialize(subElement);
               mElements.push_back(element);
            }
            else
            {
               printf(
                  "%s is not supported\n",
                  subElement->Name()
               );
            }
         }

         n = n->NextSibling();
      }
   }
}


std::vector<TmxElement *> TmxParser::getElements() const
{
   return mElements;
}


std::vector<TmxObjectGroup *> TmxParser::retrieveObjectGroups() const
{
   std::vector<TmxObjectGroup *> objectGroups;
   for (auto element : mElements)
   {
      if (element->mType == TmxElement::TypeObjectGroup)
      {
         objectGroups.push_back(dynamic_cast<TmxObjectGroup*>(element));
      }
   }

   return objectGroups;
}


TmxTileSet *TmxParser::getTileSet(TmxLayer* layer)
{
   // get maximum tile id per layer
   int tileId = 0;
   int tmpId = 0;

   for (int i = 0; i < (layer->mHeight * layer->mWidth); i++)
   {
      tmpId = layer->mData[i];

      if (tmpId > tileId)
      {
         tileId = tmpId;
      }
   }

   // std::cout << "TmxParser::getTileSet: looking up tileId: " << tileId << std::endl;

   TmxTileSet* tileset = nullptr;
   for (auto element : mElements)
   {
      if (element->mType == TmxElement::TypeTileSet)
      {
         TmxTileSet* tmp = dynamic_cast<TmxTileSet*>(element);

         if (tmp)
         {
            if (
                  tileId >= tmp->mFirstGid
               && tileId <  tmp->mFirstGid + tmp->mTileCount
            )
            {
               tileset = tmp;
               break;
            }
         }
      }
   }

   if (!tileset)
   {
     std::cout<< "TmxParser::getTileSet: loading fuckup for: " << layer->mName << std::endl;
   }

   return tileset;
}


