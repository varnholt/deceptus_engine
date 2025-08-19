#include "stenciltilemap.h"

#include <SFML/OpenGL.hpp>
#include <iostream>

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

   _stencil_shader.loadFromFile("data/shaders/stencil_write.vert", "data/shaders/stencil_write.frag");
   _stencil_shader.setUniform("u_alpha_threshold", _alpha_threshold);

   return true;
}

void StencilTileMap::draw(sf::RenderTarget& color, sf::RenderTarget& normal, sf::RenderStates states) const
{
   if (!_stencil_tilemap)
   {
      Log::Error() << "stencil is invalid";
      return;
   }

   // draw the masking geometry (stencil_tilemap) first
   _stencil_shader.setUniform("u_texture_sampler", sf::Shader::CurrentTexture);
   auto use_shader = [this]() { return _alpha_threshold < 0.99f; };

   auto stencil_render_state = states;
   stencil_render_state.shader = use_shader() ? &_stencil_shader : nullptr;
   stencil_render_state.stencilMode =  
         sf::StencilMode( // set up stencil
            {sf::StencilComparison::Always},  
            {sf::StencilUpdateOperation::Replace},
            1,
            0xff,
            true
         );

   // clear stencil buffer
   color.clearStencil(0);

   const auto visible = _stencil_tilemap->isVisible();
   _stencil_tilemap->setVisible(true);
   _stencil_tilemap->draw(color, stencil_render_state);
   _stencil_tilemap->setVisible(visible);

   // then the masked content (the tilemap)
   auto color_render_state = states;
   color_render_state.stencilMode =
         sf::StencilMode(  // set up stencil
            {sf::StencilComparison::Equal},  
            {sf::StencilUpdateOperation::Keep},
            1,
            0xff,
            false
         );

   TileMap::draw(color, normal, color_render_state);

   // debug dump
   static int32_t _frame_counter{0};
   _frame_counter += 1;
   if ((_frame_counter % 1000) == 0)
   {
      dump_composite_view_png(color, states);
   }
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

std::string iso8601_timestamp_seconds()
{
   const auto now = std::chrono::system_clock::now();
   const std::time_t tt = std::chrono::system_clock::to_time_t(now);
   std::tm tm{};
#if defined(_WIN32)
   localtime_s(&tm, &tt);
#else
   localtime_r(&tt, &tm);
#endif
   std::ostringstream oss;
   oss << std::put_time(&tm, "%Y-%m-%dT%H-%M-%S");
   return oss.str();
}

void StencilTileMap::dump_composite_view_png(sf::RenderTarget& color, const sf::RenderStates& states) const
{
   sf::ContextSettings settings;
   settings.stencilBits = 8;
   sf::RenderTexture debug_texture(color.getSize(), settings);

   // match current camera/view
   const sf::View scene_view = color.getView();
   debug_texture.setView(scene_view);
   debug_texture.clearStencil(0);
   debug_texture.clear(sf::Color::Transparent);

   // draw the color tilemap normally (no stencil test)
   {
      sf::RenderStates color_state = states;
      color_state.stencilMode = sf::StencilMode({sf::StencilComparison::Always}, {sf::StencilUpdateOperation::Keep}, 0, 0xff, false);
      TileMap::draw(debug_texture, color_state);
   }

   // write stencil bits (no color) using alpha-discard shader
   {
      _stencil_shader.setUniform("u_texture_sampler", sf::Shader::CurrentTexture);

      const bool enable_alpha_test = (_alpha_threshold < 0.999f);
      sf::RenderStates stencil_state = states;
      stencil_state.shader = enable_alpha_test ? &_stencil_shader : nullptr;
      stencil_state.stencilMode = sf::StencilMode(
         {sf::StencilComparison::Always},
         {sf::StencilUpdateOperation::Replace},
         1,
         0xff,
         true  // stencilOnly
      );

      const bool was_visible = _stencil_tilemap->isVisible();
      _stencil_tilemap->setVisible(true);
      _stencil_tilemap->draw(debug_texture, stencil_state);
      _stencil_tilemap->setVisible(was_visible);
   }

   // draw red overlay where stencil == 1
   {
      const sf::Vector2f vsize = scene_view.getSize();
      const sf::Vector2f vcenter = scene_view.getCenter();
      const sf::Vector2f topLeft{vcenter.x - 0.5f * vsize.x, vcenter.y - 0.5f * vsize.y};

      sf::RectangleShape red_rect(vsize);
      red_rect.setPosition(topLeft);
      red_rect.setFillColor(sf::Color(255, 0, 0, 128));  // half alpha

      sf::RenderStates red_state;
      red_state.stencilMode = sf::StencilMode({sf::StencilComparison::Equal}, {sf::StencilUpdateOperation::Keep}, 1, 0xff, false);
      debug_texture.draw(red_rect, red_state);
   }

   // save png
   debug_texture.display();
   std::filesystem::create_directories("debug");
   const std::string filename = "debug/composite__" + getLayerName() + "__" + iso8601_timestamp_seconds() + ".png";
   sf::Image image = debug_texture.getTexture().copyToImage();
   // image.flipVertically();
   (void)image.saveToFile(filename);
}
