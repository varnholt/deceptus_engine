#include "shaderlayer.h"

#include "framework/tools/globalclock.h"
#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "texturepool.h"

#include <iostream>


void ShaderLayer::draw(sf::RenderTarget& target)
{
   float x = mPosition.x;
   float y = mPosition.y;
   float w = mSize.x;
   float h = mSize.y;
   const auto offset = target.mapCoordsToPixel(mPosition, target.getView());

   mShader.setUniform("u_texture", *mTexture.get());
   mShader.setUniform("u_time", GlobalClock::getInstance()->getElapsedTimeInS());
   mShader.setUniform("u_resolution", sf::Vector2f(w, h));
   mShader.setUniform("u_offset", sf::Vector2f(offset.x, offset.y));

   sf::Vertex quad[] = {
      sf::Vertex(sf::Vector2f(x,     y    )),
      sf::Vertex(sf::Vector2f(x,     y + h)),
      sf::Vertex(sf::Vector2f(x + w, y + h)),
      sf::Vertex(sf::Vector2f(x + w, y    ))
   };

   sf::RenderStates states;
   states.shader = &mShader;


   target.draw(quad, 4, sf::Quads, states);
}


std::shared_ptr<ShaderLayer> ShaderLayer::deserialize(TmxObject* object)
{
   std::shared_ptr<ShaderLayer> instance = std::make_shared<ShaderLayer>();

   instance->mPosition.x = object->mX;
   instance->mPosition.y = object->mY;
   instance->mSize.x = object->mWidth;
   instance->mSize.y = object->mHeight;

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
         instance->mTexture->setRepeated(true);
      }
   }

   return instance;
}
