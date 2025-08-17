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

   _stencil_shader.loadFromFile("data/shaders/stencil_write.frag", sf::Shader::Type::Fragment);
   _stencil_shader.setUniform("u_alphaThreshold", _alpha_threshold);

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
   _stencil_shader.setUniform("u_tex", sf::Shader::CurrentTexture);
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
   // color.clearStencil(0);
   const auto old_view = color.getView();
   color.setView(color.getDefaultView());
   color.clearStencil(0);
   color.setView(old_view);

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

void StencilTileMap::dump_both_tilemaps_png() const
{
   // ensure output folder exists
   std::filesystem::create_directories("debug");

   const std::string ts = iso8601_timestamp_seconds();

   const std::string color_layer = getLayerName();
   const std::string stencil_layer = _stencil_tilemap ? _stencil_tilemap->getLayerName() : "stencil_missing";

   const std::filesystem::path color_png = std::filesystem::path("debug") / (color_layer + "__color__" + ts + ".png");
   const std::filesystem::path stencil_png = std::filesystem::path("debug") / (stencil_layer + "__stencil__" + ts + ".png");

   (void)TileMap::dumpToPng(color_png);

   if (_stencil_tilemap)
   {
      (void)_stencil_tilemap->dumpToPng(stencil_png);
   }
}

#include <SFML/OpenGL.hpp>

void StencilTileMap::dump_color_target_png(sf::RenderTarget& color, const std::string& prefix) const
{
   std::filesystem::create_directories("debug");

   const sf::Vector2u size = color.getSize();
   if (size.x == 0 || size.y == 0)
      return;

   const std::string filename = "debug/" + prefix + "__" + getLayerName() + "__" + iso8601_timestamp_seconds() + ".png";

   // Fast path: offscreen color target is a RenderTexture
   if (auto* rt = dynamic_cast<sf::RenderTexture*>(&color))
   {
      // SFML 3: ensure the texture is up-to-date
      rt->display();
      const sf::Image img = rt->getTexture().copyToImage();
      (void)img.saveToFile(filename);
      return;
   }

   // On-screen path: color target is a RenderWindow -> snapshot via texture.update(window)
   if (auto* win = dynamic_cast<sf::RenderWindow*>(&color))
   {
      // SFML 3: construct texture with size (no create())
      sf::Texture snapshot(size);
      snapshot.update(*win);  // grabs exactly what's in the window's framebuffer
      const sf::Image img = snapshot.copyToImage();
      (void)img.saveToFile(filename);
      return;
   }

   // If you ever use another RenderTarget subtype, add a branch here.
}

void StencilTileMap::dump_composite_view_png(sf::RenderTarget& color, const sf::RenderStates& states) const
{
   if (!_stencil_tilemap)
      return;

   const sf::Vector2u size = color.getSize();
   if (size.x == 0u || size.y == 0u)
      return;

   // Local offscreen RT (SFML 3: construct with size + settings; no create())
   sf::ContextSettings settings;
   settings.depthBits = 24;
   settings.stencilBits = 8;  // we need stencil for the overlay
   sf::RenderTexture composite_rt(size, settings);

   // Clear color + stencil over the full target
   composite_rt.setView(composite_rt.getDefaultView());
   composite_rt.clear(sf::Color::Transparent);
   composite_rt.clearStencil(0);

   // Match the app’s current camera/view
   const sf::View scene_view = color.getView();
   composite_rt.setView(scene_view);

   // PASS A: draw the COLOR tilemap normally (no stencil test)
   {
      sf::RenderStates color_state = states;
      color_state.stencilMode = sf::StencilMode({sf::StencilComparison::Always}, {sf::StencilUpdateOperation::Keep}, 0, 0xff, false);
      // single-target overload: only color content
      TileMap::draw(composite_rt, color_state);
   }

   // PASS B: write stencil bits (no color) using alpha-discard shader
   {
      _stencil_shader.setUniform("u_tex", sf::Shader::CurrentTexture);

      sf::RenderStates stencil_state = states;
      stencil_state.shader = &_stencil_shader;
      stencil_state.texture = nullptr;  // let the stencil tilemap bind its own atlas
      stencil_state.stencilMode = sf::StencilMode(
         {sf::StencilComparison::Always},
         {sf::StencilUpdateOperation::Replace},
         1,
         0xff,
         true  // stencilOnly: write stencil, don’t write color
      );

      const bool was_visible = _stencil_tilemap->isVisible();
      _stencil_tilemap->setVisible(true);
      _stencil_tilemap->draw(composite_rt, stencil_state);  // single-target draw
      _stencil_tilemap->setVisible(was_visible);
   }

   // PASS C: draw RED overlay where stencil == 1 (on TOP so you see it)
   {
      const sf::Vector2f vsize = scene_view.getSize();
      const sf::Vector2f vcenter = scene_view.getCenter();
      const sf::Vector2f topLeft{vcenter.x - 0.5f * vsize.x, vcenter.y - 0.5f * vsize.y};

      sf::RectangleShape red_rect(vsize);
      red_rect.setPosition(topLeft);
      red_rect.setFillColor(sf::Color(255, 0, 0, 128));  // half alpha

      sf::RenderStates red_state;
      red_state.stencilMode = sf::StencilMode({sf::StencilComparison::Equal}, {sf::StencilUpdateOperation::Keep}, 1, 0xff, false);
      composite_rt.draw(red_rect, red_state);
   }

   // Save PNG
   composite_rt.display();
   std::filesystem::create_directories("debug");
   const std::string filename = "debug/composite__" + getLayerName() + "__" + iso8601_timestamp_seconds() + ".png";
   const sf::Image image = composite_rt.getTexture().copyToImage();
   (void)image.saveToFile(filename);
}
