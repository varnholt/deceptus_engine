#pragma once

#include "tilemap.h"

/*!
 \brief tilemap that draws itself only where another tilemap has written to the stencil buffer.

\details
draw() uses a two-pass pipeline on the color target:
1) write 1s to the stencil where the mask tilemap renders (no color writes);
   if _alpha_threshold < 0.99f an alpha-test shader discards pixels below the threshold.
2) draw this tilemap where stencil == 1.
- the stencil buffer is cleared to 0 at the start of each draw().
- the same stencil state is forwarded to the normal target via TileMap::draw(color, normal, states),
so lighting stays consistent.
- requires sfml 3 and a render target created with stencilBits > 0.
- if no stencil tilemap is set, nothing is drawn (an error is logged).
- dumpStencilAndColorToPng() can write a diagnostic composite image for debugging.

usage:
- in tiled, set "stencil_reference" on the content layer.
- after loading layers, call setStencilTilemap(mask_layer_ptr) on the content layer.
- set _alpha_threshold in [0..1]; use >= 0.99f to disable per-pixel masking (whole-tile stenciling).
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
   void dumpStencilAndColorToPng(sf::RenderTarget& color, const sf::RenderStates& states) const;

   std::string _stencil_reference;
   std::shared_ptr<TileMap> _stencil_tilemap = nullptr;
   mutable sf::Shader _stencil_shader;
   float _alpha_threshold{0.5f};
};
