#pragma once

#include "tilemap.h"

/*! \brief A tile map implementation that contains a second tilemap that serves as a stencil buffer
 *
  */
class StencilTileMap : public TileMap
{
   public:

      StencilTileMap() = default;

      bool load(TmxLayer* layer, TmxTileSet* tileset, const std::filesystem::path& base_path) override;
      void draw(sf::RenderTarget& color, sf::RenderTarget& normal, sf::RenderStates states) const override;


   private:

      void prepareWriteToStencilBuffer() const;
      void prepareWriteColor() const;
      void disableStencilTest() const;

      TileMap* _stencil_tilemap = nullptr;
};

