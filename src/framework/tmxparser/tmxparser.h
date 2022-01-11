#pragma once

#include <string>
#include <vector>

#include "tinyxml2/tinyxml2.h"

struct TmxElement;
struct TmxLayer;
struct TmxObjectGroup;
struct TmxTileSet;

class TmxParser
{
   public:

      TmxParser() = default;

      void parse(const std::string& filename);
      std::vector<TmxElement *> getElements() const;
      std::vector<TmxObjectGroup*> retrieveObjectGroups() const;
      TmxTileSet* getTileSet(TmxLayer *layer);

   protected:

      std::vector<TmxElement*> _elements;
      void parseGroup(tinyxml2::XMLElement* sub_element, int32_t& z);
      void parseSubElement(tinyxml2::XMLElement* sub_element, int32_t& z);

      std::string _filename;
};

