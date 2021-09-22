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
      tinyxml2::XMLElement* doc_element = doc.FirstChildElement();
      tinyxml2::XMLNode* n = doc_element->FirstChild();

      while(n != nullptr)
      {
         tinyxml2::XMLElement* subElement = n->ToElement();
         if (subElement != nullptr)
         {
            TmxElement* element = nullptr;
            TmxLayer* layer = nullptr;

            if (subElement->Name() == std::string("tileset"))
            {
               element = new TmxTileSet();
               dynamic_cast<TmxTileSet*>(element)->_path = std::filesystem::path(filename).parent_path();
            }
            else if (subElement->Name() == std::string("layer"))
            {
               element = new TmxLayer();
               layer = dynamic_cast<TmxLayer*>(element);
            }
            else if (subElement->Name() == std::string("imagelayer"))
            {
               element = new TmxImageLayer();
               dynamic_cast<TmxImageLayer*>(element)->_z = z;
            }
            else if (subElement->Name() == std::string("objectgroup"))
            {
               element = new TmxObjectGroup();
               dynamic_cast<TmxObjectGroup*>(element)->_z_index = z;
            }

            if (element != nullptr)
            {
               element->deserialize(subElement);

               if (layer && layer->_properties)
               {
                  auto& map = layer->_properties->_map;
                  auto it = map.find("z");
                  if (it != map.end())
                  {
                     z = it->second->_value_int.value();
                  }
                  dynamic_cast<TmxLayer*>(element)->_z = z;
               }

               // std::cout << "layer: " << element->mName << " z: " << z << std::endl;

               _elements.push_back(element);
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


std::vector<TmxElement*> TmxParser::getElements() const
{
   return _elements;
}


std::vector<TmxObjectGroup *> TmxParser::retrieveObjectGroups() const
{
   std::vector<TmxObjectGroup *> object_groups;
   for (auto element : _elements)
   {
      if (element->_type == TmxElement::TypeObjectGroup)
      {
         object_groups.push_back(dynamic_cast<TmxObjectGroup*>(element));
      }
   }

   return object_groups;
}


TmxTileSet *TmxParser::getTileSet(TmxLayer* layer)
{
   // get maximum tile id per layer
   int tild_id = 0;
   int tmp_id = 0;

   for (auto i = 0u; i < (layer->_height_px * layer->_width_px); i++)
   {
      tmp_id = layer->_data[i];

      if (tmp_id > tild_id)
      {
         tild_id = tmp_id;
      }
   }

   // std::cout << "TmxParser::getTileSet: looking up tileId: " << tileId << std::endl;

   TmxTileSet* tileset = nullptr;
   for (auto element : _elements)
   {
      if (element->_type == TmxElement::TypeTileSet)
      {
         auto tmp = dynamic_cast<TmxTileSet*>(element);

         if (tmp)
         {
            if (
                  tild_id >= tmp->_first_gid
               && tild_id <  tmp->_first_gid + tmp->_tile_count
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
     // std::cout<< "TmxParser::getTileSet: loading fuckup for: " << layer->mName << std::endl;
   }

   return tileset;
}


