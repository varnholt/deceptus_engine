#include "tmxtemplate.h"

#include "tinyxml2/tinyxml2.h"

#include "framework/tools/log.h"
#include "tmxobject.h"


TmxTemplate::TmxTemplate(const std::string& filename)
{
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

            if (sub_element->Name() == std::string("object"))
            {
                _object = new TmxObject();
                _object->deserialize(sub_element);
            }

            node = node->NextSibling();
        }
    }
}
