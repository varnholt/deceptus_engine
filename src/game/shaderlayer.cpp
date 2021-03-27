#include "shaderlayer.h"

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "texturepool.h"

#include <iostream>


std::shared_ptr<ShaderLayer> ShaderLayer::deserialize(TmxObject* object)
{
   std::shared_ptr<ShaderLayer> image = std::make_shared<ShaderLayer>();

   // image->mTexture = TexturePool::getInstance().get((levelPath / imageLayer->mImage->mSource).string());
   // image->mSprite.setPosition(imageLayer->mOffsetX, imageLayer->mOffsetY);
   // image->mSprite.setTexture(*image->mTexture);

   if (object->mProperties != nullptr)
   {
      auto z = object->mProperties->mMap.find("z");
      if (z != object->mProperties->mMap.end())
      {
         image->mZ = object->mProperties->mMap["z"]->mValueInt.value();
      }
   }

   return image;
}
