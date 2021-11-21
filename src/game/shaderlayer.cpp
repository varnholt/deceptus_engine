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
   float x = _position.x;
   float y = _position.y;
   float w = _size.x;
   float h = _size.y;

   _shader.setUniform("u_uv_height", _uv_height);
   _shader.setUniform("u_texture", *_texture.get());
   _shader.setUniform("u_time", GlobalClock::getInstance().getElapsedTimeInS() + _time_offset);
   _shader.setUniform("u_resolution", sf::Vector2f(w, h));

   sf::Vertex quad[] = {
      sf::Vertex(sf::Vector2f(x,     y    ), sf::Vector2f(0.0f,     _uv_height)),
      sf::Vertex(sf::Vector2f(x,     y + h), sf::Vector2f(0.0f,     0.0f)),
      sf::Vertex(sf::Vector2f(x + w, y + h), sf::Vector2f(_uv_width, 0.0f)),
      sf::Vertex(sf::Vector2f(x + w, y    ), sf::Vector2f(_uv_width, _uv_height))
   };

   sf::RenderStates states;
   states.shader = &_shader;
   states.blendMode = sf::BlendAlpha;

   target.draw(quad, 4, sf::Quads, states);
}


std::shared_ptr<ShaderLayer> ShaderLayer::deserialize(TmxObject* object)
{
   std::shared_ptr<ShaderLayer> instance = std::make_shared<ShaderLayer>();

   instance->_position.x = object->_x_px;
   instance->_position.y = object->_y_px;
   instance->_size.x = object->_width_px;
   instance->_size.y = object->_height_px;

   if (object->_properties != nullptr)
   {
      auto z = object->_properties->_map.find("z");
      if (z != object->_properties->_map.end())
      {
         instance->_z = z->second->_value_int.value();
      }

      auto uv_width_it = object->_properties->_map.find("uv_width");
      if (uv_width_it != object->_properties->_map.end())
      {
         instance->_uv_width = uv_width_it->second->_value_float.value();
      }

      auto uv_height_it = object->_properties->_map.find("uv_height");
      if (uv_height_it != object->_properties->_map.end())
      {
         instance->_uv_height = uv_height_it->second->_value_float.value();
      }

      auto time_offset_it = object->_properties->_map.find("time_offset_s");
      if (time_offset_it != object->_properties->_map.end())
      {
         instance->_time_offset = time_offset_it->second->_value_float.value();
      }

      auto vertex_shader_it = object->_properties->_map.find("vertex_shader");
      if (vertex_shader_it != object->_properties->_map.end())
      {
         instance->_shader.loadFromFile(vertex_shader_it->second->_value_string.value(), sf::Shader::Vertex);
      }

      auto fragment_shader_it = object->_properties->_map.find("fragment_shader");
      if (fragment_shader_it != object->_properties->_map.end())
      {
         instance->_shader.loadFromFile(fragment_shader_it->second->_value_string.value(), sf::Shader::Fragment);
      }

      auto texture_id = object->_properties->_map.find("texture");
      if (texture_id != object->_properties->_map.end())
      {
         instance->_texture = TexturePool::getInstance().get(texture_id->second->_value_string.value());
         instance->_texture->setRepeated(true);
      }
   }

   return instance;
}
