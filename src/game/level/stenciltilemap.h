#pragma once

#include "tilemap.h"

/*! \brief A tile map implementation that contains a second tilemap that serves as a stencil buffer
 *
 */
class StencilTileMap : public TileMap
{
public:
   StencilTileMap() = default;

   bool load(const std::shared_ptr<TmxLayer>& layer, const std::shared_ptr<TmxTileSet>& tileset, const std::filesystem::path& base_path)
      override;
   void draw(sf::RenderTarget& color, sf::RenderTarget& normal, sf::RenderStates states) const override;

   const std::string& getStencilReference() const;
   void setStencilTilemap(const std::shared_ptr<TileMap>& stencil_tilemap);

private:
   void prepareWriteToStencilBuffer() const;
   void prepareWriteColor() const;
   void disableStencilTest() const;

   std::string _stencil_reference;
   std::shared_ptr<TileMap> _stencil_tilemap = nullptr;
};
