#include "shaderlayer.h"

#include "framework/tools/globalclock.h"
#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "texturepool.h"

#include <iostream>


void ShaderLayer::draw(sf::RenderTarget& target)
{
   mRenderTexture.clear();

   mShader.setUniform("texture", *mTexture.get());
   mShader.setUniform("time", GlobalClock::getInstance()->getElapsedTimeInS());

   mRenderTexture.draw(mSprite, &mShader);
   mRenderTexture.display();

   target.draw(mSprite);
}


std::shared_ptr<ShaderLayer> ShaderLayer::deserialize(TmxObject* object)
{
   std::shared_ptr<ShaderLayer> instance = std::make_shared<ShaderLayer>();

   instance->mRenderTexture.create(static_cast<uint32_t>(object->mWidth), static_cast<uint32_t>(object->mHeight));
   instance->mSprite.setPosition(object->mX, object->mY);

   if (object->mProperties != nullptr)
   {
      auto z = object->mProperties->mMap.find("z");
      if (z != object->mProperties->mMap.end())
      {
         instance->mZ = z->second->mValueInt.value();
      }

      // shader
      auto vertex_shader_it = object->mProperties->mMap.find("vertex_shader");
      if (vertex_shader_it != object->mProperties->mMap.end())
      {
         instance->mShader.loadFromFile(vertex_shader_it->second->mValueStr.value(), sf::Shader::Vertex);
      }

      auto fragment_shader_it = object->mProperties->mMap.find("fragment_shader");
      if (fragment_shader_it != object->mProperties->mMap.end())
      {
         instance->mShader.loadFromFile(fragment_shader_it->second->mValueStr.value(), sf::Shader::Fragment);
      }

      // texture uniform
      auto texture_id = object->mProperties->mMap.find("texture");
      if (texture_id != object->mProperties->mMap.end())
      {
         instance->mTexture = TexturePool::getInstance().get(texture_id->second->mValueStr.value());
      }
   }

   return instance;
}
