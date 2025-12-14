#include "shaderlayer.h"

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "game/io/texturepool.h"
#include "game/io/valuereader.h"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace
{
std::map<std::string, std::function<ShaderLayer::FactoryFunction>>& getCustomizations()
{
   static std::map<std::string, std::function<ShaderLayer::FactoryFunction>> __customizations;
   return __customizations;
}
}  // namespace

void ShaderLayer::registerCustomization(const std::string& id, const std::function<FactoryFunction>& factory_function)
{
   getCustomizations()[id] = factory_function;
}

ShaderLayer::ShaderLayer(GameNode* parent) : GameNode(parent)
{
   setClassName(typeid(ShaderLayer).name());
}

std::string_view ShaderLayer::objectName() const
{
   return "ShaderLayer";
}

void ShaderLayer::checkShaderUniforms(const std::string& shaderPath)
{
   std::ifstream file(shaderPath);
   if (!file.is_open())
   {
      return;
   }

   std::stringstream buffer;
   buffer << file.rdbuf();
   const auto shader_source = buffer.str();

   _has_u_resolution = shader_source.find("u_resolution;") != std::string::npos;
   _has_u_uv_height = shader_source.find("u_uv_height;") != std::string::npos;
}

void ShaderLayer::draw(sf::RenderTarget& target, sf::RenderTarget& /*normal*/)
{
   const auto x = _position.x;
   const auto y = _position.y;
   const auto w = _size.x;
   const auto h = _size.y;

   _shader.setUniform("u_texture", *_texture.get());
   _shader.setUniform("u_time", _elapsed.asSeconds() + _time_offset);

   if (_has_u_resolution)
   {
      _shader.setUniform("u_resolution", sf::Vector2f(w, h));
   }

   if (_has_u_uv_height)
   {
      _shader.setUniform("u_uv_height", _uv_height);
   }

   sf::Vertex quad[] = {
      sf::Vertex(sf::Vector2f(x, y), sf::Color::White, sf::Vector2f(0.0f, _uv_height)),
      sf::Vertex(sf::Vector2f(x, y + h), sf::Color::White, sf::Vector2f(0.0f, 0.0f)),
      sf::Vertex(sf::Vector2f(x + w, y), sf::Color::White, sf::Vector2f(_uv_width, _uv_height)),
      sf::Vertex(sf::Vector2f(x + w, y + h), sf::Color::White, sf::Vector2f(_uv_width, 0.0f))
   };

   sf::RenderStates states;
   states.shader = &_shader;
   states.blendMode = sf::BlendAlpha;

   target.draw(quad, 4, sf::PrimitiveType::TriangleStrip, states);
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
   if (!data._tmx_object->_properties)
   {
      Log::Error() << "shader layer does not have any properties, id: " << data._tmx_object->_name;
      return nullptr;
   }

   const auto& map = data._tmx_object->_properties->_map;

   // shader layers can have customization implementations in cpp that drive the shader uniforms
   std::shared_ptr<ShaderLayer> instance;
   const auto customization = ValueReader::readValue<std::string>("customization", map);
   if (customization.has_value())
   {
      instance = getCustomizations()[customization.value()](parent);
   }
   else
   {
      instance = std::make_shared<ShaderLayer>(parent);
   }

   const auto bounding_rect =
      sf::FloatRect{{data._tmx_object->_x_px, data._tmx_object->_y_px}, {data._tmx_object->_width_px, data._tmx_object->_height_px}};

   instance->_position.x = data._tmx_object->_x_px;
   instance->_position.y = data._tmx_object->_y_px;
   instance->_size.x = data._tmx_object->_width_px;
   instance->_size.y = data._tmx_object->_height_px;
   instance->setObjectId(data._tmx_object->_name);
   instance->_rect = bounding_rect;
   instance->addChunks(bounding_rect);
   instance->_z_index = ValueReader::readValue<int32_t>("z", map).value_or(instance->_z_index);
   instance->_uv_width = ValueReader::readValue<float>("uv_width", map).value_or(instance->_uv_width);
   instance->_uv_height = ValueReader::readValue<float>("uv_height", map).value_or(instance->_uv_height);
   instance->_time_offset = ValueReader::readValue<float>("time_offset_s", map).value_or(instance->_time_offset);

   const auto vert_file = ValueReader::readValue<std::string>("vertex_shader", map);
   if (vert_file.has_value())
   {
      // Check if vertex shader file exists before attempting to load
      if (!std::filesystem::exists(vert_file.value()))
      {
         Log::Error() << "vertex shader file does not exist: " << vert_file.value();
      }
      else if (!instance->_shader.loadFromFile(vert_file.value(), sf::Shader::Type::Vertex))
      {
         Log::Error() << "error compiling " << vert_file.value();
      }
   }

   const auto frag_file = ValueReader::readValue<std::string>("fragment_shader", map);
   if (frag_file.has_value())
   {
      // Check if fragment shader file exists before attempting to load
      if (!std::filesystem::exists(frag_file.value()))
      {
         Log::Error() << "fragment shader file does not exist: " << frag_file.value();
      }
      else if (!instance->_shader.loadFromFile(frag_file.value(), sf::Shader::Type::Fragment))
      {
         Log::Error() << "error compiling " << frag_file.value();
      }

      // Analyze the fragment shader source to determine which uniforms are present
      instance->checkShaderUniforms(frag_file.value());
   }

   const auto texture_id = ValueReader::readValue<std::string>("texture", map);
   if (texture_id.has_value())
   {
      instance->_texture = TexturePool::getInstance().get(texture_id.value());
      instance->_texture->setRepeated(true);

      const auto smooth_texture = ValueReader::readValue<bool>("smooth_texture", map).value_or(false);
      instance->_texture->setSmooth(smooth_texture);
   }

   return instance;
}
