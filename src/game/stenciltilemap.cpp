#include "stenciltilemap.h"

#include <SFML/OpenGL.hpp>

#include "framework/tools/log.h"
#include "framework/tmxparser/tmxlayer.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"


bool StencilTileMap::load(TmxLayer* layer, TmxTileSet* tileset, const std::filesystem::path& base_path)
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

   return true;
}


void StencilTileMap::draw(sf::RenderTarget& color, sf::RenderTarget& normal, sf::RenderStates states) const
{
   prepareWriteToStencilBuffer();
   const auto visible = _stencil_tilemap->isVisible();
   _stencil_tilemap->setVisible(true);
   _stencil_tilemap->draw(color, states);
   _stencil_tilemap->setVisible(visible);

   prepareWriteColor();
   TileMap::draw(color, normal, states);

   disableStencilTest();
}


void StencilTileMap::prepareWriteToStencilBuffer() const
{
   glClear(GL_STENCIL_BUFFER_BIT);
   glEnable(GL_STENCIL_TEST);

   glAlphaFunc(GL_GREATER, 0.5f);                       // SFML renders every alpha value by default
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);   // so we configure alpha testing to kick out
   glEnable(GL_ALPHA_TEST);                             // all lower alpha values

   glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); // don't render to the color buffers
   glStencilFunc(GL_ALWAYS, 1, 0xFF);                   // place a 1 wherever we render stuff
   glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);     // replace where rendered
}


void StencilTileMap::prepareWriteColor() const
{
   glAlphaFunc(GL_ALWAYS, 0.0f);

   glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);      // write to the color buffers
   glStencilFunc(GL_EQUAL, 1, 0xFF);                     // where a 1 was put into the buffer
   glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);               // keep the contents
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
