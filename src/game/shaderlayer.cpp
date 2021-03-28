#include "shaderlayer.h"

#include "camerapane.h"
#include "framework/tools/globalclock.h"
#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "texturepool.h"

#include <iostream>


void ShaderLayer::draw(sf::RenderTarget& target)
{
   const auto look_vector = CameraPane::getInstance().getLookVector();

   float x = _position.x;
   float y = _position.y;
   float w = _size.x;
   float h = _size.y;

   // not really sure why the look vector y coordinate inside the camera view is not
   // part of the target view in the first place. workaround: '2 * look_vector.y'
   const auto offset_fix = 2 * look_vector.y;
   const auto position_offset = target.mapCoordsToPixel(sf::Vector2f(x, y + offset_fix), target.getView());

   _shader.setUniform("u_texture", *_texture.get());
   _shader.setUniform("u_time", GlobalClock::getInstance()->getElapsedTimeInS());
   _shader.setUniform("u_resolution", sf::Vector2f(w, h));
   _shader.setUniform("u_offset", sf::Vector2f(static_cast<float>(position_offset.x), static_cast<float>(position_offset.y)));

   sf::Vertex quad[] = {
      sf::Vertex(sf::Vector2f(x,     y    )),
      sf::Vertex(sf::Vector2f(x,     y + h)),
      sf::Vertex(sf::Vector2f(x + w, y + h)),
      sf::Vertex(sf::Vector2f(x + w, y    ))
   };

   sf::RenderStates states;
   states.shader = &_shader;
   states.blendMode = sf::BlendAlpha;

   target.draw(quad, 4, sf::Quads, states);
}


std::shared_ptr<ShaderLayer> ShaderLayer::deserialize(TmxObject* object)
{
   std::shared_ptr<ShaderLayer> instance = std::make_shared<ShaderLayer>();

   instance->_position.x = object->mX;
   instance->_position.y = object->mY;
   instance->_size.x = object->mWidth;
   instance->_size.y = object->mHeight;

   if (object->mProperties != nullptr)
   {
      auto z = object->mProperties->mMap.find("z");
      if (z != object->mProperties->mMap.end())
      {
         instance->_z = z->second->mValueInt.value();
      }

      // shader
      auto vertex_shader_it = object->mProperties->mMap.find("vertex_shader");
      if (vertex_shader_it != object->mProperties->mMap.end())
      {
         instance->_shader.loadFromFile(vertex_shader_it->second->mValueStr.value(), sf::Shader::Vertex);
      }

      auto fragment_shader_it = object->mProperties->mMap.find("fragment_shader");
      if (fragment_shader_it != object->mProperties->mMap.end())
      {
         instance->_shader.loadFromFile(fragment_shader_it->second->mValueStr.value(), sf::Shader::Fragment);
      }

      // texture uniform
      auto texture_id = object->mProperties->mMap.find("texture");
      if (texture_id != object->mProperties->mMap.end())
      {
         instance->_texture = TexturePool::getInstance().get(texture_id->second->mValueStr.value());
         instance->_texture->setRepeated(true);
      }
   }

   return instance;
}
