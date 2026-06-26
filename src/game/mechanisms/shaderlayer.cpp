#include "shaderlayer.h"

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "game/io/texturepool.h"
#include "game/io/valuereader.h"

#include <filesystem>
#include <span>

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

void ShaderLayer::checkUniforms()
{
   if (!_shader)
   {
      return;
   }
   const auto get_loc = [this](const char* name) -> std::optional<sf::Shader::UniformLocation>
   {
      auto result = _shader->getUniformLocation(name);
      if (result.hasValue())
      {
         return *result;
      }
      return std::nullopt;
   };
   _u_texture_loc = get_loc("u_texture");
   _u_time_loc = get_loc("u_time");
   _u_resolution_loc = get_loc("u_resolution");
   _u_uv_height_loc = get_loc("u_uv_height");
}

void ShaderLayer::draw(sf::RenderTarget& target, sf::RenderTarget& /*normal*/)
{
   if (!_shader)
   {
      return;
   }

   const auto x = _position.x;
   const auto y = _position.y;
   const auto w = _size.x;
   const auto h = _size.y;

   if (_u_texture_loc && _texture)
   {
      (void)_shader->setUniform(*_u_texture_loc, *_texture.get());
   }
   if (_u_time_loc)
   {
      _shader->setUniform(*_u_time_loc, _elapsed.asSeconds() + _time_offset);
   }
   if (_u_resolution_loc)
   {
      _shader->setUniform(*_u_resolution_loc, sf::Glsl::Vec2{w, h});
   }
   if (_u_uv_height_loc)
   {
      _shader->setUniform(*_u_uv_height_loc, _uv_height);
   }

   const sf::Vertex quad[] = {
      sf::Vertex{.position = {x, y}, .color = sf::Color::White, .texCoords = {0.0f, _uv_height}},
      sf::Vertex{.position = {x, y + h}, .color = sf::Color::White, .texCoords = {0.0f, 0.0f}},
      sf::Vertex{.position = {x + w, y}, .color = sf::Color::White, .texCoords = {_uv_width, _uv_height}},
      sf::Vertex{.position = {x + w, y + h}, .color = sf::Color::White, .texCoords = {_uv_width, 0.0f}}
   };

   target.draw(
      std::span<const sf::Vertex>{quad, 4},
      sf::PrimitiveType::TriangleStrip,
      sf::RenderStates{.blendMode = sf::BlendAlpha, .shader = _shader.get()}
   );
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
   const auto frag_file = ValueReader::readValue<std::string>("fragment_shader", map);
   if (vert_file.has_value() || frag_file.has_value())
   {
      sf::Shader::LoadFromFileSettings shader_settings;
      if (vert_file.has_value())
      {
         // Check if vertex shader file exists before attempting to load
         if (!std::filesystem::exists(vert_file.value()))
         {
            Log::Error() << "vertex shader file does not exist: " << vert_file.value();
         }
         else
         {
            shader_settings.vertexPath = vert_file.value();
         }
      }
      if (frag_file.has_value())
      {
         // check if fragment shader file exists before attempting to load
         if (!std::filesystem::exists(frag_file.value()))
         {
            Log::Error() << "fragment shader file does not exist: " << frag_file.value();
         }
         else
         {
            shader_settings.fragmentPath = frag_file.value();
         }
      }
      auto loaded = sf::Shader::loadFromFile(shader_settings);
      if (loaded.hasValue())
      {
         instance->_shader = std::make_unique<sf::Shader>(std::move(*loaded));
         instance->checkUniforms();
      }
      else
      {
         Log::Error() << "error loading shader";
      }
   }

   const auto texture_id = ValueReader::readValue<std::string>("texture", map);
   if (texture_id.has_value())
   {
      instance->_texture = TexturePool::getInstance().get(texture_id.value());
      instance->_texture->setRepeated(true);

      const auto smooth_texture = ValueReader::readValue<bool>("smooth_texture", map).value_or(false);
      instance->_texture->setSmooth(smooth_texture);
   }

   instance->readCustomProperties(data);

   return instance;
}
