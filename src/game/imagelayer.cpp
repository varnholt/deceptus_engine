#include "imagelayer.h"

#include "framework/tmxparser/tmximage.h"
#include "framework/tmxparser/tmximagelayer.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "texturepool.h"

#include <iostream>


//-----------------------------------------------------------------------------
std::shared_ptr<ImageLayer> ImageLayer::deserialize(TmxElement* element, const std::filesystem::path& levelPath)
{
  std::shared_ptr<ImageLayer> image = std::make_shared<ImageLayer>();
  auto imageLayer = dynamic_cast<TmxImageLayer*>(element);

  image->mZ = imageLayer->_z;
  image->mTexture = TexturePool::getInstance().get((levelPath / imageLayer->_image->_source).string());
  image->mSprite.setPosition(imageLayer->_offset_x_px, imageLayer->_offset_y_px);
  image->mSprite.setColor(sf::Color(255, 255, 255, static_cast<uint32_t>(imageLayer->_opacity * 255.0f)));
  image->mSprite.setTexture(*image->mTexture);

  sf::BlendMode blendMode = sf::BlendAdd;
  if (imageLayer->_properties != nullptr)
  {
     auto z = imageLayer->_properties->_map.find("z");
     if (z != imageLayer->_properties->_map.end())
     {
        image->mZ = imageLayer->_properties->_map["z"]->_value_int.value();
        // std::cout << "image layer has z: " << image->mZ << std::endl;
     }

     std::string blendModeStr;
     auto it = imageLayer->_properties->_map.find("blendmode");
     if (it != imageLayer->_properties->_map.end())
     {
        blendModeStr = it->second->_value_string.value();

        if (blendModeStr == "alpha")
        {
           blendMode = sf::BlendAlpha;
        }
        else if (blendModeStr == "multiply")
        {
           blendMode = sf::BlendMultiply;
        }
        else if (blendModeStr == "add")
        {
           blendMode = sf::BlendAdd;
        }
        else if (blendModeStr == "none")
        {
           blendMode = sf::BlendNone;
        }
     }
  }

  image->mBlendMode = blendMode;

  return image;
}

