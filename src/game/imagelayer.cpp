#include "imagelayer.h"

#include "tmxparser/tmximage.h"
#include "tmxparser/tmximagelayer.h"
#include "tmxparser/tmxproperties.h"
#include "tmxparser/tmxproperty.h"


//-----------------------------------------------------------------------------
std::shared_ptr<ImageLayer> ImageLayer::deserialize(TmxElement* element, const std::filesystem::path& levelPath)
{
  std::shared_ptr<ImageLayer> image = std::make_shared<ImageLayer>();
  auto imageLayer = dynamic_cast<TmxImageLayer*>(element);
  image->mZ = imageLayer->mZ;
  image->mTexture.loadFromFile((levelPath / imageLayer->mImage->mSource).string());
  image->mSprite.setPosition(imageLayer->mOffsetX, imageLayer->mOffsetY);
  image->mSprite.setColor(sf::Color(255, 255, 255, static_cast<uint32_t>(imageLayer->mOpacity * 255.0f)));
  image->mSprite.setTexture(image->mTexture);

  sf::BlendMode blendMode = sf::BlendAdd;
  if (imageLayer->mProperties != nullptr)
  {
     std::string blendModeStr;
     auto it = imageLayer->mProperties->mMap.find("blendmode");
     if (it != imageLayer->mProperties->mMap.end())
     {
        blendModeStr = it->second->mValueStr;

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

