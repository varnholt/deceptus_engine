#pragma once

#include <memory>
#include <string>
#include <vector>

#include "tinyxml2/tinyxml2.h"

struct TmxElement;
struct TmxLayer;
struct TmxObjectGroup;
struct TmxParseData;
struct TmxTileSet;

class TmxParser
{
   public:

      TmxParser() = default;
      virtual ~TmxParser();

      void parse(const std::string& filename);
      const std::vector<TmxElement*>& getElements() const;
      std::vector<TmxObjectGroup*> retrieveObjectGroups() const;
      TmxTileSet* getTileSet(TmxLayer *layer) const;

   protected:

      std::vector<TmxElement*> _elements;
      void parseGroup(tinyxml2::XMLElement* sub_element, int32_t& z);
      void parseSubElement(tinyxml2::XMLElement* sub_element, int32_t& z);

      std::shared_ptr<TmxParseData> _parse_data;
};

