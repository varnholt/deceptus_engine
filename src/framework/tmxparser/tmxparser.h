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

      void parse(const std::string& filename);
      const std::vector<std::shared_ptr<TmxElement>>& getElements() const;
      std::vector<std::shared_ptr<TmxObjectGroup>> retrieveObjectGroups() const;
      std::shared_ptr<TmxTileSet> getTileSet(const std::shared_ptr<TmxLayer>& layer) const;

   protected:

      std::vector<std::shared_ptr<TmxElement>> _elements;
      void parseGroup(tinyxml2::XMLElement* sub_element, int32_t& z);
      void parseSubElement(tinyxml2::XMLElement* sub_element, int32_t& z);

      std::shared_ptr<TmxParseData> _parse_data;
};

