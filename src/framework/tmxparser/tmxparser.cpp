#include "tmxparser.h"

#include "framework/tools/log.h"

#include "tmximagelayer.h"
#include "tmxobjectgroup.h"
#include "tmxlayer.h"
#include "tmxtileset.h"
#include "tmxparsedata.h"
#include "tmxproperty.h"
#include "tmxproperties.h"

#include <iostream>


void TmxParser::parse(const std::string& filename)
{
   _parse_data = std::make_shared<TmxParseData>();
   _parse_data->_filename = filename;

   auto z = 0;

   tinyxml2::XMLDocument doc;
   if (doc.LoadFile(filename.c_str()) == tinyxml2::XML_SUCCESS)
   {
      auto doc_element = doc.FirstChildElement();
      auto node = doc_element->FirstChild();

      while (node)
      {
         auto sub_element = node->ToElement();
         if (!sub_element)
         {
            node = node->NextSibling();
            continue;
         }

         // groups are just flattened
         if (sub_element->Name() == std::string("group"))
         {
            parseGroup(sub_element, z);
         }
         else
         {
            parseSubElement(sub_element, z);
         }

         node = node->NextSibling();
      }
   }
}


void TmxParser::parseGroup(tinyxml2::XMLElement* sub_element, int32_t& z)
{
   auto nested_child = sub_element->FirstChild();
   while (nested_child)
   {
      // std::cout << "parse " << nested_child->ToElement()->Name() << std::endl;
      parseSubElement(nested_child->ToElement(), z);
      nested_child = nested_child->NextSibling();
   };
}


void TmxParser::parseSubElement(tinyxml2::XMLElement* sub_element, int32_t& z)
{
   std::shared_ptr<TmxElement> sub_element_parsed;
   std::shared_ptr<TmxLayer> layer;

   if (sub_element->Name() == std::string("tileset"))
   {
      sub_element_parsed = std::make_shared<TmxTileSet>();
      std::dynamic_pointer_cast<TmxTileSet>(sub_element_parsed)->_path = std::filesystem::path(_parse_data->_filename).parent_path();
   }
   else if (sub_element->Name() == std::string("layer"))
   {
      sub_element_parsed = std::make_shared<TmxLayer>();
      layer = std::dynamic_pointer_cast<TmxLayer>(sub_element_parsed);
   }
   else if (sub_element->Name() == std::string("imagelayer"))
   {
      sub_element_parsed = std::make_shared<TmxImageLayer>();
      std::dynamic_pointer_cast<TmxImageLayer>(sub_element_parsed)->_z = z;
   }
   else if (sub_element->Name() == std::string("objectgroup"))
   {
      sub_element_parsed = std::make_shared<TmxObjectGroup>();
      std::dynamic_pointer_cast<TmxObjectGroup>(sub_element_parsed)->_z_index = z;
   }
   else if (sub_element->Name() == std::string("group"))
   {
      parseGroup(sub_element, z);
   }

   if (!sub_element_parsed)
   {
      Log::Error() << sub_element->Name() << " is not supported";
      return;
   }

   sub_element_parsed->deserialize(sub_element, _parse_data);

   if (layer && layer->_properties)
   {
      auto& map = layer->_properties->_map;
      auto it = map.find("z");
      if (it != map.end())
      {
         z = it->second->_value_int.value();
      }
      std::dynamic_pointer_cast<TmxLayer>(sub_element_parsed)->_z = z;
   }

   _elements.push_back(sub_element_parsed);
}


const std::vector<std::shared_ptr<TmxElement>>& TmxParser::getElements() const
{
   return _elements;
}


std::vector<std::shared_ptr<TmxObjectGroup>> TmxParser::retrieveObjectGroups() const
{
   std::vector<std::shared_ptr<TmxObjectGroup>> object_groups;
   for (auto element : _elements)
   {
      if (element->_type == TmxElement::Type::TypeObjectGroup)
      {
         object_groups.push_back(std::dynamic_pointer_cast<TmxObjectGroup>(element));
      }
   }

   return object_groups;
}


std::shared_ptr<TmxTileSet> TmxParser::getTileSet(const std::shared_ptr<TmxLayer>& layer) const
{
   // get maximum tile id per layer
   int32_t tile_id = 0;
   int32_t tmp_id = 0;

   for (auto i = 0u; i < (layer->_height_tl * layer->_width_tl); i++)
   {
      tmp_id = layer->_data[i];

      if (tmp_id > tile_id)
      {
         tile_id = tmp_id;
      }
   }

   std::shared_ptr<TmxTileSet> tileset;
   for (auto element : _elements)
   {
      if (element->_type == TmxElement::Type::TypeTileSet)
      {
         auto tmp = std::dynamic_pointer_cast<TmxTileSet>(element);

         if (tmp)
         {
            if (
                  tile_id >= tmp->_first_gid
               && tile_id <  tmp->_first_gid + tmp->_tile_count
            )
            {
               tileset = tmp;
               break;
            }
         }
      }
   }

   return tileset;
}


