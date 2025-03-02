#include "shaderlayer.h"

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "game/io/texturepool.h"
#include "game/io/valuereader.h"

ShaderLayer::ShaderLayer(GameNode* parent) : GameNode(parent)
{
   setClassName(typeid(ShaderLayer).name());
}

void ShaderLayer::draw(sf::RenderTarget& target, sf::RenderTarget& /*normal*/)
{
   const auto x = _position.x;
   const auto y = _position.y;
   const auto w = _size.x;
   const auto h = _size.y;

   _shader.setUniform("u_uv_height", _uv_height);
   _shader.setUniform("u_texture", *_texture.get());
   _shader.setUniform("u_time", _elapsed.asSeconds() + _time_offset);
   _shader.setUniform("u_resolution", sf::Vector2f(w, h));

   sf::Vertex quad[] = {
      sf::Vertex(sf::Vector2f(x, y), sf::Vector2f(0.0f, _uv_height)),
      sf::Vertex(sf::Vector2f(x, y + h), sf::Vector2f(0.0f, 0.0f)),
      sf::Vertex(sf::Vector2f(x + w, y + h), sf::Vector2f(_uv_width, 0.0f)),
      sf::Vertex(sf::Vector2f(x + w, y), sf::Vector2f(_uv_width, _uv_height))
   };

   sf::RenderStates states;
   states.shader = &_shader;
   states.blendMode = sf::BlendAlpha;

   target.draw(quad, 4, sf::Quads, states);
}

void ShaderLayer::update(const sf::Time& dt)
{
   _elapsed += dt;
}

std::optional<sf::FloatRect> ShaderLayer::getBoundingBoxPx()
{
   return _rect;
}

std::shared_ptr<ShaderLayer> ShaderLayer::deserialize(GameNode* parent, const GameDeserializeData& data)
{
   std::shared_ptr<ShaderLayer> instance = std::make_shared<ShaderLayer>(parent);

   const auto bounding_rect =
      sf::FloatRect{data._tmx_object->_x_px, data._tmx_object->_y_px, data._tmx_object->_width_px, data._tmx_object->_height_px};

   instance->_position.x = data._tmx_object->_x_px;
   instance->_position.y = data._tmx_object->_y_px;
   instance->_size.x = data._tmx_object->_width_px;
   instance->_size.y = data._tmx_object->_height_px;
   instance->setObjectId(data._tmx_object->_name);
   instance->_rect = bounding_rect;
   instance->addChunks(bounding_rect);

   if (data._tmx_object->_properties)
   {
      auto z = data._tmx_object->_properties->_map.find("z");
      if (z != data._tmx_object->_properties->_map.end())
      {
         instance->_z_index = z->second->_value_int.value();
      }

      auto uv_width_it = data._tmx_object->_properties->_map.find("uv_width");
      if (uv_width_it != data._tmx_object->_properties->_map.end())
      {
         instance->_uv_width = uv_width_it->second->_value_float.value();
      }

      auto uv_height_it = data._tmx_object->_properties->_map.find("uv_height");
      if (uv_height_it != data._tmx_object->_properties->_map.end())
      {
         instance->_uv_height = uv_height_it->second->_value_float.value();
      }

      auto time_offset_it = data._tmx_object->_properties->_map.find("time_offset_s");
      if (time_offset_it != data._tmx_object->_properties->_map.end())
      {
         instance->_time_offset = time_offset_it->second->_value_float.value();
      }

      auto vertex_shader_it = data._tmx_object->_properties->_map.find("vertex_shader");
      if (vertex_shader_it != data._tmx_object->_properties->_map.end())
      {
         const auto vert_file = vertex_shader_it->second->_value_string.value();
         if (!instance->_shader.loadFromFile(vert_file, sf::Shader::Vertex))
         {
            Log::Error() << "error compiling " << vert_file;
         }
      }

      auto fragment_shader_it = data._tmx_object->_properties->_map.find("fragment_shader");
      if (fragment_shader_it != data._tmx_object->_properties->_map.end())
      {
         const auto frag_file = fragment_shader_it->second->_value_string.value();
         if (!instance->_shader.loadFromFile(frag_file, sf::Shader::Type::Fragment))
         {
            Log::Error() << "error compiling " << frag_file;
         }
      }

      auto texture_id = data._tmx_object->_properties->_map.find("texture");
      if (texture_id != data._tmx_object->_properties->_map.end())
      {
         instance->_texture = TexturePool::getInstance().get(texture_id->second->_value_string.value());
         instance->_texture->setRepeated(true);

         const auto& map = data._tmx_object->_properties->_map;
         const auto smooth_texture = ValueReader::readValue<bool>("smooth_texture", map).value_or(false);
         instance->_texture->setSmooth(smooth_texture);
      }
   }

   return instance;
}
