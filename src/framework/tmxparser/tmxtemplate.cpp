#include "tmxtemplate.h"

#include <filesystem>
#include <iostream>
#include "tinyxml2/tinyxml2.h"

#include "framework/tools/assetsource.h"
#include "framework/tools/log.h"
#include "tmxobject.h"
#include "tmxparsedata.h"

TmxTemplate::TmxTemplate(const std::string& filename, const std::shared_ptr<TmxParseData>& parse_data)
{
   const auto path = (std::filesystem::path(parse_data->_filename).parent_path() / filename).lexically_normal();

   tinyxml2::XMLDocument doc;
   const auto file_contents = AssetSource::readFile(path);
   if (file_contents.has_value() && doc.Parse(file_contents->c_str(), file_contents->size()) == tinyxml2::XML_SUCCESS)
   {
      auto* doc_element = doc.FirstChildElement();
      auto* node = doc_element->FirstChild();

      while (node)
      {
         auto* sub_element = node->ToElement();
         if (!sub_element)
         {
            node = node->NextSibling();
            continue;
         }

         if (sub_element->Name() == std::string("object"))
         {
            _object = std::make_shared<TmxObject>();
            _object->deserialize(sub_element, parse_data);
         }

         node = node->NextSibling();
      }
   }
}
