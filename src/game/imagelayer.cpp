#include "imagelayer.h"

#include "framework/tmxparser/tmximage.h"
#include "framework/tmxparser/tmximagelayer.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "texturepool.h"

#include <iostream>


//-----------------------------------------------------------------------------
std::shared_ptr<ImageLayer> ImageLayer::deserialize(const std::shared_ptr<TmxElement>& element, const std::filesystem::path& level_path)
{
  std::shared_ptr<ImageLayer> image = std::make_shared<ImageLayer>();
  auto image_layer = std::dynamic_pointer_cast<TmxImageLayer>(element);

  image->_z_index = image_layer->_z;
  image->_texture = TexturePool::getInstance().get((level_path / image_layer->_image->_source).string());
  image->_sprite.setPosition(image_layer->_offset_x_px, image_layer->_offset_y_px);
  image->_sprite.setColor(sf::Color(255, 255, 255, static_cast<uint32_t>(image_layer->_opacity * 255.0f)));
  image->_sprite.setTexture(*image->_texture);

  sf::BlendMode blend_mode = sf::BlendAdd;
  if (image_layer->_properties)
  {
     auto z = image_layer->_properties->_map.find("z");
     if (z != image_layer->_properties->_map.end())
     {
        image->_z_index = image_layer->_properties->_map["z"]->_value_int.value();
     }

     auto it = image_layer->_properties->_map.find("blendmode");
     if (it != image_layer->_properties->_map.end())
     {
        std::string blend_mode_str;
        blend_mode_str = it->second->_value_string.value();

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
  }

  image->_blend_mode = blend_mode;

  return image;
}

