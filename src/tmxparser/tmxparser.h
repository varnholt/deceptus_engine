#pragma once

#include <vector>

struct TmxLayer;
struct TmxObjectGroup;
struct TmxTileSet;

// forward declarations
struct TmxElement;

class TmxParser
{
   public:

      TmxParser();

      void parse(const std::string& filename);
      std::vector<TmxElement *> getElements() const;
      std::vector<TmxObjectGroup*> retrieveObjectGroups() const;
      TmxTileSet* getTileSet(TmxLayer *layer);

   protected:

      std::vector<TmxElement*> mElements;
};

