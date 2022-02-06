#include "tilemapfactory.h"

#include "stenciltilemap.h"
#include "framework/tmxparser/tmxlayer.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tools/log.h"


std::shared_ptr<TileMap> TileMapFactory::makeTileMap(TmxLayer* layer)
{
   std::shared_ptr<TileMap> tile_map;

   // detect tilemaps that use a stencil layer
   if (layer->_properties)
   {
      const auto it = layer->_properties->_map.find("stencil_reference");
      if (it != layer->_properties->_map.end())
      {
         tile_map = std::make_shared<StencilTileMap>();
      }
   }

   // make default tilemap
   if (!tile_map)
   {
      tile_map = std::make_shared<TileMap>();
   }

   return tile_map;
}


void TileMapFactory::merge(const std::vector<std::shared_ptr<TileMap>>& tile_maps)
{
   std::map<std::string, std::shared_ptr<TileMap>> tile_maps_map;
   for (auto& tile_map : tile_maps)
   {
      tile_maps_map[tile_map->getLayerName()] = tile_map;
   }

   for (auto& tile_map : tile_maps)
   {
      auto stencil_tile_map = dynamic_pointer_cast<StencilTileMap>(tile_map);

      if (!stencil_tile_map)
      {
         continue;
      }

      const auto reference_name = stencil_tile_map->getStencilReference();
      auto reference_map = tile_maps_map[reference_name];

      if (!reference_map)
      {
         Log::Error() << "stencil reference map (" << reference_name << ") is not available";
         continue;
      }

      stencil_tile_map->setStencilTilemap(reference_map);
   }
}
