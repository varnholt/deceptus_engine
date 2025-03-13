#include "imagelayer.h"

#include "framework/tmxparser/tmximage.h"
#include "framework/tmxparser/tmximagelayer.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "game/io/texturepool.h"
#include "player/player.h"

ImageLayer::ImageLayer(GameNode* parent) : GameNode(parent)
{
}

void ImageLayer::draw(sf::RenderTarget& target, sf::RenderTarget& /*normal*/)
{
   if (!_sprite)
   {
      return;
   }

   const auto& level_view = target.getView();

   if (_parallax_settings.has_value())
   {
      target.setView(_parallax_view);
   }

   target.draw(*_sprite, {_blend_mode});

   if (_parallax_settings.has_value())
   {
      target.setView(level_view);
   }
}

void ImageLayer::update(const sf::Time& dt)
{
   const auto& player_chunk = Player::getCurrent()->getChunk();
   _texture->update(player_chunk);

   if (auto texture = _texture->getTexture(); texture.has_value())
   {
      if (_sprite == nullptr)
      {
         _sprite = std::make_unique<sf::Sprite>(texture->get());
         _sprite->setPosition(_position);
         _sprite->setColor(_color);
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

   _parallax_view = sf::View{
      {level_view_x * (*_parallax_settings)._factor.x + (*_parallax_settings)._error.x,
       level_view_y * (*_parallax_settings)._factor.y + (*_parallax_settings)._error.y},
      {view_width, view_height}
   };
}

void ImageLayer::resetView(float view_width, float view_height)
{
   _parallax_view = sf::View{sf::FloatRect({0.0f, 0.0f}, {view_width, view_height})};
   _parallax_view.setViewport(sf::FloatRect({0.0f, 0.0f}, {1.0f, 1.0f}));
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
   image->_texture = TexturePool::getInstance().get((level_path / image_layer->_image->_source).string());
   image->_sprite = std::make_unique<sf::Sprite>(*image->_texture);
   image->_sprite->setPosition({image_layer->_offset_x_px, image_layer->_offset_y_px});
   image->_sprite->setColor(sf::Color(255, 255, 255, static_cast<uint8_t>(image_layer->_opacity * 255.0f)));

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
   image->_texture = std::make_unique<LazyTexture>(texture_path, image->_chunks);
   image->_position = {image_layer->_offset_x_px, image_layer->_offset_y_px};
   image->_color = {255, 255, 255, static_cast<uint8_t>(image_layer->_opacity * 255.0f)};

   return image;
}
