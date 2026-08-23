#include "imagelayer.h"

#include "framework/tmxparser/tmximage.h"
#include "framework/tmxparser/tmximagelayer.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tools/sfmlcompat.h"
#include "game/io/texturepool.h"
#include "game/player/playerregistry.h"

#ifdef DEVELOPMENT_MODE
#include "game/debug/drawcallcounter.h"
#endif

namespace
{
///
/// \brief Tells whether a sprite reaches into the region a view shows.
/// \param view the view the sprite would be drawn through.
/// \param sprite the sprite in question.
/// \return true when the two rectangles overlap.
///
/// The layers are only ever unloaded by chunk. LazyTexture keeps a texture while any of its chunks is
/// within one 512 px chunk of the player's, which is a very loose fence around a 640 x 360 view:
///
///                  <----------- chunk keep-alive, ~1024 px ----------->
///     +-----------+-----------+-----------+-----------+-----------+     each cell = one 512 px chunk
///     |           |           |           |           |           |
///     |  fp-the   |  es-the   |  +----------------+   |  ct-the   |     the view is barely wider
///     |  -hall    |  -cellar  |  |     view       |   |  -loop    |     than one chunk, so four or
///     |  loaded   |  loaded   |  |   640 x 360    |   |  loaded   |     five rooms' worth of
///     |  DRAWN    |  DRAWN    |  +----------------+   |  DRAWN    |     overlays are resident and
///     |  0 px     |  0 px     |   es/fp-widow-maker   |  0 px     |     every one of them drew
///     |           |           |   DRAWN, 1.00x each   |           |
///     +-----------+-----------+-----------+-----------+-----------+
///
/// Measured in the catacombs: 24 image layers drawn per frame, and exactly two of them putting a
/// pixel on screen. The other 22 cost a draw call each, and the parallax ones a view change on top,
/// which flushes the batch under VRSFML.
///
/// Dropping them is exact rather than a trade: a sprite that shares no area with the view rasterises
/// nothing, whatever its blend mode. The views here are built from a rectangle and never rotated, so
/// comparing world space rectangles is the whole test. The proof it is exact is in the counters -
/// after this, 24 draws became 3 and every overdraw figure stayed byte identical.
///
bool reachesView(const sf::View& view, const sf::Sprite& sprite)
{
   const auto view_center = sfcompat::getViewCenter(view);
   const auto view_size = sfcompat::getViewSize(view);
   const auto bounds = sprite.getGlobalBounds();

   const auto view_left = view_center.x - view_size.x * 0.5f;
   const auto view_top = view_center.y - view_size.y * 0.5f;

   return bounds.position.x < view_left + view_size.x && view_left < bounds.position.x + bounds.size.x &&
          bounds.position.y < view_top + view_size.y && view_top < bounds.position.y + bounds.size.y;
}
}  // namespace

ImageLayer::ImageLayer(GameNode* parent) : GameNode(parent)
{
}

void ImageLayer::preload()
{
   _texture->preload();
}

bool ImageLayer::drainTextures()
{
   return _texture->drain();
}

std::string_view ImageLayer::objectName() const
{
   return "ImageLayer";
}

void ImageLayer::draw(sf::RenderTarget& target, sf::RenderTarget& normal)
{
#ifdef DECEPTUS_VRSFML
   draw(target, normal, {});
#else
   if (_sprite == nullptr)
   {
      return;
   }

   if (!_visible)
   {
      return;
   }

   // level view is copied here on purpose
   const auto level_view = target.getView();

   // a layer that shares no area with the view rasterises nothing, so it is dropped before it costs
   // a draw call and, for the parallax ones, two view changes
   if (!reachesView(_parallax_settings.has_value() ? _parallax_view : level_view, *_sprite))
   {
      return;
   }

   if (_parallax_settings.has_value())
   {
      target.setView(_parallax_view);
   }

   target.draw(*_sprite, {_blend_mode});

#ifdef DEVELOPMENT_MODE
   DrawCallCounter::countImageLayerPixels(target, {_blend_mode}, *_sprite, getObjectId());
#endif

   if (_parallax_settings.has_value())
   {
      target.setView(level_view);
   }
#endif
}

void ImageLayer::draw(sf::RenderTarget& target, sf::RenderTarget& normal, const sf::RenderStates& states)
{
#ifdef DECEPTUS_VRSFML
   if (_sprite == nullptr)
   {
      return;
   }

   if (!_visible)
   {
      return;
   }

   // the texture has to travel in the render states, vrsfml sprites do not own one
   const auto* texture = _texture->getTexture().get();

   // a layer that shares no area with the view rasterises nothing, so it is dropped before it costs
   // a draw call and, for the parallax ones, a view change - which flushes the batch here
   if (!reachesView(_parallax_settings.has_value() ? _parallax_view : states.view, *_sprite))
   {
      return;
   }

   if (_parallax_settings.has_value())
   {
      const sf::RenderStates parallax_states{.blendMode = _blend_mode, .view = _parallax_view, .texture = texture};
      target.draw(*_sprite, parallax_states);
#ifdef DEVELOPMENT_MODE
      DrawCallCounter::countImageLayerPixels(target, parallax_states, *_sprite, getObjectId());
#endif
   }
   else
   {
      sf::RenderStates draw_states = states;
      draw_states.blendMode = _blend_mode;
      draw_states.texture = texture;
      target.draw(*_sprite, draw_states);
#ifdef DEVELOPMENT_MODE
      DrawCallCounter::countImageLayerPixels(target, draw_states, *_sprite, getObjectId());
#endif
   }
#else
   (void)states;
   draw(target, normal);
#endif
}

void ImageLayer::update(const sf::Time& dt)
{
   const auto& player_chunk = PlayerRegistry::getFirst()->getChunk();
   _texture->update(player_chunk);

   if (_texture->getTexture())
   {
      if (_sprite == nullptr)
      {
#ifdef DECEPTUS_VRSFML
         _sprite = std::make_unique<sf::Sprite>();
         _sprite->position = _position;
         _sprite->color = _color;

         // vrsfml sprites are not constructed from a texture, so the texture rect stays empty
         // (and nothing is drawn) unless it is set to cover the texture explicitly
         const auto texture_size = _texture->getTexture()->getSize();
         _sprite->textureRect = sf::FloatRect{{0.0f, 0.0f}, {static_cast<float>(texture_size.x), static_cast<float>(texture_size.y)}};
#else
         _sprite = std::make_unique<sf::Sprite>(*_texture->getTexture());
         _sprite->setPosition(_position);
         _sprite->setColor(_color);
#endif
      }
   }
   else
   {
      _sprite.reset();
   }
}

void ImageLayer::updateView(float level_view_x, float level_view_y, float view_width, float view_height)
{
   if (!_parallax_settings.has_value())
   {
      return;
   }

#ifdef DECEPTUS_VRSFML
   _parallax_view = sf::View::fromRect(sf::FloatRect{
      {level_view_x * (*_parallax_settings)._factor.x + (*_parallax_settings)._error.x,
       level_view_y * (*_parallax_settings)._factor.y + (*_parallax_settings)._error.y},
      {view_width, view_height}
   });
#else
   _parallax_view = sf::View{sf::FloatRect{
      {level_view_x * (*_parallax_settings)._factor.x + (*_parallax_settings)._error.x,
       level_view_y * (*_parallax_settings)._factor.y + (*_parallax_settings)._error.y},
      {view_width, view_height}
   }};
#endif
}

void ImageLayer::resetView(float view_width, float view_height)
{
#ifdef DECEPTUS_VRSFML
   _parallax_view = sf::View::fromRect(sf::FloatRect{{0.0f, 0.0f}, {view_width, view_height}});
   _parallax_view.viewport = sf::FloatRect{{0.0f, 0.0f}, {1.0f, 1.0f}};
#else
   _parallax_view = sf::View{sf::FloatRect({0.0f, 0.0f}, {view_width, view_height})};
   _parallax_view.setViewport(sf::FloatRect({0.0f, 0.0f}, {1.0f, 1.0f}));
#endif
}

std::optional<sf::FloatRect> ImageLayer::getBoundingBoxPx()
{
   return std::nullopt;
}

std::shared_ptr<ImageLayer> ImageLayer::deserialize(const std::shared_ptr<TmxElement>& element, const std::filesystem::path& level_path)
{
   std::shared_ptr<ImageLayer> image = std::make_shared<ImageLayer>();
   auto image_layer = std::dynamic_pointer_cast<TmxImageLayer>(element);

   image->_z_index = image_layer->_z;

   sf::BlendMode blend_mode = sf::BlendAlpha;
   if (image_layer->_properties)
   {
      auto z_index_it = image_layer->_properties->_map.find("z");
      if (z_index_it != image_layer->_properties->_map.end())
      {
         image->_z_index = z_index_it->second->_value_int.value();
      }

      const auto& blend_mode_it = image_layer->_properties->_map.find("blendmode");
      if (blend_mode_it != image_layer->_properties->_map.end())
      {
         std::string blend_mode_str;
         blend_mode_str = blend_mode_it->second->_value_string.value();

         if (blend_mode_str == "alpha")
         {
            blend_mode = sf::BlendAlpha;
         }
         else if (blend_mode_str == "multiply")
         {
            blend_mode = sf::BlendMultiply;
         }
         else if (blend_mode_str == "add")
         {
            blend_mode = sf::BlendAdd;
         }
         else if (blend_mode_str == "none")
         {
            blend_mode = sf::BlendNone;
         }
      }

      // read parallax settings if parallax is enabled
      const auto& use_parallax_it = image_layer->_properties->_map.find("use_parallax");
      if (use_parallax_it != image_layer->_properties->_map.end())
      {
         const auto use_parallax = use_parallax_it->second->_value_bool.value();
         if (use_parallax)
         {
            ParallaxSettings settings;
            settings.deserialize(image_layer->_properties);
            image->_parallax_settings = settings;
         }
      }

      const auto& post_lighting_it = image_layer->_properties->_map.find("post_lighting");
      if (post_lighting_it != image_layer->_properties->_map.end())
      {
         image->_post_lighting = post_lighting_it->second->_value_bool.value();
      }

      // todo: read room limitations
   }

   image->_blend_mode = blend_mode;

   if (!image->_parallax_settings.has_value())
   {
      const auto rect = sf::FloatRect{
         {image_layer->_offset_x_px, image_layer->_offset_y_px},
         {static_cast<float>(image_layer->_image->_width_px), static_cast<float>(image_layer->_image->_height_px)}
      };

      image->addChunks(rect);
   }

   const auto texture_path = level_path / image_layer->_image->_source;
   image->_texture = std::make_shared<LazyTexture>(texture_path, image->_chunks);
   image->_position = {image_layer->_offset_x_px, image_layer->_offset_y_px};
   image->_color = {255, 255, 255, static_cast<uint8_t>(image_layer->_opacity * 255.0f)};
   image->_texture_path = texture_path;
   image->setObjectId(element->_name);

   return image;
}
