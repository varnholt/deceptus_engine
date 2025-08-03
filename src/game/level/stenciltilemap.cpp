#include "stenciltilemap.h"

#include <SFML/OpenGL.hpp>

#include "framework/tmxparser/tmxlayer.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tools/log.h"

bool StencilTileMap::load(
   const std::shared_ptr<TmxLayer>& layer,
   const std::shared_ptr<TmxTileSet>& tileset,
   const std::filesystem::path& base_path
)
{
   TileMap::load(layer, tileset, base_path);

   if (!layer->_properties)
   {
      Log::Error() << "stencil layer does not have any properties";
      return false;
   }

   const auto it = layer->_properties->_map.find("stencil_reference");
   if (it == layer->_properties->_map.end())
   {
      Log::Error() << "stencil layer does not have 'stencil_reference' property";
      return false;
   }

   _stencil_reference = (*it).second->_value_string.value();

   if (_stencil_reference == getLayerName())
   {
      Log::Error() << "no, no, no, dude, you cannot set the 'stencil_reference' to itself";
      return false;
   }

   _alphaDiscardShader.loadFromFile("alpha_discard.frag", sf::Shader::Type::Fragment);

   return true;
}

void StencilTileMap::draw2(sf::RenderTarget& color, sf::RenderTarget& normal, sf::RenderStates states) const
{
   // 1. Clear stencil buffer (must use raw GL unless SFML exposes a helper)
   glClear(GL_STENCIL_BUFFER_BIT);  // Still needed until SFML wraps this

   // 2. Draw stencil tilemap to stencil buffer only
   sf::RenderStates stencilWriteStates = states;
   stencilWriteStates.shader = &_alphaDiscardShader;
   stencilWriteStates.stencilMode.stencilComparison = sf::StencilComparison::Always;
   stencilWriteStates.stencilMode.stencilUpdateOperation = sf::StencilUpdateOperation::Replace;
   stencilWriteStates.stencilMode.stencilReference = 1;
   stencilWriteStates.stencilMode.stencilMask = 0xFF;
   stencilWriteStates.stencilMode.stencilOnly = true;

   const bool wasVisible = _stencil_tilemap->isVisible();
   _stencil_tilemap->setVisible(true);
   _stencil_tilemap->draw(color, stencilWriteStates);
   _stencil_tilemap->setVisible(wasVisible);

   // 3. Draw this tilemap where stencil == 1
   sf::RenderStates drawStates = states;
   drawStates.stencilMode.stencilComparison = sf::StencilComparison::Equal;
   drawStates.stencilMode.stencilUpdateOperation = sf::StencilUpdateOperation::Keep;
   drawStates.stencilMode.stencilReference = 1;
   drawStates.stencilMode.stencilMask = 0xFF;
   drawStates.stencilMode.stencilOnly = false;

   TileMap::draw(color, normal, drawStates);
}

void StencilTileMap::saveStencilBufferDebugImage(const std::string& filename, const sf::RenderTarget& target) const
{
   const auto size = target.getSize();
   std::vector<uint8_t> buffer(size.x * size.y);

   // SFML 3 does not expose stencil readback, so we use OpenGL directly.
   glReadPixels(0, 0, size.x, size.y, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, buffer.data());

   sf::Image image(sf::Vector2u{size.x, size.y});

   for (unsigned int y = 0; y < size.y; ++y)
   {
      for (unsigned int x = 0; x < size.x; ++x)
      {
         std::size_t i = (size.y - 1 - y) * size.x + x;
         uint8_t stencilValue = buffer[i];

         // visualize: white if stencil is 1, black if 0
         sf::Color color = stencilValue > 0 ? sf::Color::White : sf::Color::Black;
         image.setPixel({x, y}, color);
      }
   }

   if (!image.saveToFile(filename))
   {
      Log::Error() << "Failed to save stencil debug image: " << filename;
   }
   else
   {
      Log::Info() << "Saved stencil debug image to: " << filename;
   }
}

void StencilTileMap::draw(sf::RenderTarget& color, sf::RenderTarget& normal, sf::RenderStates states) const
{
   draw2(color, normal, states);

   // prepareWriteToStencilBuffer();
   // const auto visible = _stencil_tilemap->isVisible();
   // _stencil_tilemap->setVisible(true);
   // _stencil_tilemap->draw(color, states);
   // _stencil_tilemap->setVisible(visible);

   // prepareWriteColor();
   // TileMap::draw(color, normal, states);

   // disableStencilTest();
}

void StencilTileMap::prepareWriteToStencilBuffer() const
{
   glClear(GL_STENCIL_BUFFER_BIT);
   glEnable(GL_STENCIL_TEST);

   glAlphaFunc(GL_GREATER, 0.5f);                      // SFML renders every alpha value by default
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  // so we configure alpha testing to kick out
   glEnable(GL_ALPHA_TEST);                            // all lower alpha values

   glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);  // don't render to the color buffers
   glStencilFunc(GL_ALWAYS, 1, 0xFF);                    // place a 1 wherever we render stuff
   glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);      // replace where rendered
}

void StencilTileMap::prepareWriteColor() const
{
   glAlphaFunc(GL_ALWAYS, 0.0f);

   glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);  // write to the color buffers
   glStencilFunc(GL_EQUAL, 1, 0xFF);                 // where a 1 was put into the buffer
   glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);           // keep the contents
}

void StencilTileMap::disableStencilTest() const
{
   glDisable(GL_STENCIL_TEST);
}

void StencilTileMap::setStencilTilemap(const std::shared_ptr<TileMap>& stencil_tilemap)
{
   _stencil_tilemap = stencil_tilemap;
}

const std::string& StencilTileMap::getStencilReference() const
{
   return _stencil_reference;
}
