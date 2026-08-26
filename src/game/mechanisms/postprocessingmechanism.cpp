#include "postprocessingmechanism.h"

#include <array>
#include <filesystem>
#include <sstream>

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tools/log.h"
#include "game/io/texturepool.h"
#include "game/io/valuereader.h"
#include "game/mechanisms/gamemechanismdeserializerconstants.h"
#include "game/mechanisms/gamemechanismdeserializerregistry.h"
#include "game/player/playerregistry.h"

namespace
{
//! \brief prefix marking a tmx property as a shader uniform
constexpr std::string_view uniform_prefix{"u_"};

static constexpr std::array post_processing_scopes{
   std::string_view{"all"},
   std::string_view{"level"},
};

static constexpr std::array post_processing_properties{
   PropertyInfo{.name = "fragment_shader", .type = "string", .default_value = "", .required = true},
   PropertyInfo{.name = "vertex_shader", .type = "string", .default_value = ""},
   PropertyInfo{.name = "scope", .type = "string", .default_value = "all", .allowed_values = post_processing_scopes},
   PropertyInfo{.name = "z", .type = "int", .default_value = int32_t{0}},
};

static constexpr MechanismSchema post_processing_schema{
   .type_name = "PostProcessing",
   .layer_name = "post_processing",
   .default_width = 480,
   .default_height = 270,
   .properties = post_processing_properties,
};

const auto registered_post_processing = []
{
   auto& registry = GameMechanismDeserializerRegistry::instance();
   registry.registerSchema(post_processing_schema);

   registry.mapGroupToLayer("PostProcessing", "post_processing");

   registry.registerLayerName(
      "post_processing",
      [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms)
      {
         auto mechanism = PostProcessingMechanism::deserialize(parent, data);
         if (mechanism)
         {
            mechanisms["post_processing"]->push_back(mechanism);
         }
      }
   );

   registry.registerObjectGroup(
      "PostProcessing",
      [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms)
      {
         auto mechanism = PostProcessingMechanism::deserialize(parent, data);
         if (mechanism)
         {
            mechanisms["post_processing"]->push_back(mechanism);
         }
      }
   );
   return true;
}();

//! \brief splits a comma separated string into floats so vec2/vec3/vec4 uniforms can be written as text
std::vector<float> parseFloatList(const std::string& text)
{
   std::vector<float> values;
   std::stringstream stream(text);
   std::string token;
   while (std::getline(stream, token, ','))
   {
      try
      {
         values.push_back(std::stof(token));
      }
      catch (...)
      {
         return {};
      }
   }
   return values;
}

}  // namespace

PostProcessingMechanism::PostProcessingMechanism(GameNode* parent) : GameNode(parent)
{
   setClassName(typeid(PostProcessingMechanism).name());
   _render_stage = MechanismRenderStage::PostProcessing;
}

std::string_view PostProcessingMechanism::objectName() const
{
   return "PostProcessing";
}

void PostProcessingMechanism::update(const sf::Time& dt)
{
   _elapsed_s += dt.asSeconds();

   if (!_has_trigger_area)
   {
      return;
   }

   const auto player = PlayerRegistry::getFirst();
   if (!player)
   {
      return;
   }

   setEnabled(_rect.contains(player->getPixelPositionFloat()));
}

std::optional<sf::FloatRect> PostProcessingMechanism::getBoundingBoxPx()
{
   return _rect;
}

PostProcessing::Scope PostProcessingMechanism::getScope() const
{
   return _scope;
}

const sf::Shader* PostProcessingMechanism::prepare(const sf::Texture& texture)
{
   if (!_shader.isLoaded())
   {
      return nullptr;
   }

   PostProcessing::applyBuiltInUniforms(_shader, texture, _elapsed_s);

   // anything configured in tmx wins over the built-ins above
   for (const auto& uniform : _uniforms)
   {
      std::visit(
         [this, &uniform](const auto& value)
         {
            using ValueType = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<ValueType, std::shared_ptr<sf::Texture>>)
            {
               if (value)
               {
                  _shader.setUniform(uniform._name, *value);
               }
            }
            else
            {
               _shader.setUniform(uniform._name, value);
            }
         },
         uniform._value
      );
   }

   return &_shader.native();
}

std::shared_ptr<PostProcessingMechanism> PostProcessingMechanism::deserialize(GameNode* parent, const GameDeserializeData& data)
{
   if (!data._tmx_object->_properties)
   {
      Log::Error() << "post processing mechanism has no properties, id: " << data._tmx_object->_name;
      return nullptr;
   }

   const auto& map = data._tmx_object->_properties->_map;

   auto instance = std::make_shared<PostProcessingMechanism>(parent);
   instance->setObjectId(data._tmx_object->_name);
   instance->_z_index = ValueReader::readValue<int32_t>("z", map).value_or(instance->_z_index);

   instance->_rect =
      sf::FloatRect{{data._tmx_object->_x_px, data._tmx_object->_y_px}, {data._tmx_object->_width_px, data._tmx_object->_height_px}};

   // a sized rectangle gates the effect on the player being inside it, a zero-sized one leaves the
   // mechanism disabled until a script enables it
   instance->_has_trigger_area = (data._tmx_object->_width_px > 0.0f && data._tmx_object->_height_px > 0.0f);
   if (instance->_has_trigger_area)
   {
      instance->addChunks(instance->_rect);
   }
   instance->setEnabled(false);

   const auto scope = ValueReader::readValue<std::string>("scope", map);
   if (scope.has_value())
   {
      const auto parsed_scope = PostProcessing::scopeFromName(scope.value());
      if (parsed_scope.has_value())
      {
         instance->_scope = parsed_scope.value();
      }
      else
      {
         Log::Error() << "unknown post processing scope: " << scope.value();
      }
   }

   const auto vertex_file = ValueReader::readValue<std::string>("vertex_shader", map);
   const auto fragment_file = ValueReader::readValue<std::string>("fragment_shader", map);

   const auto vertex_exists = vertex_file.has_value() && std::filesystem::exists(vertex_file.value());
   const auto fragment_exists = fragment_file.has_value() && std::filesystem::exists(fragment_file.value());

   if (vertex_file.has_value() && !vertex_exists)
   {
      Log::Error() << "vertex shader file does not exist: " << vertex_file.value();
   }
   if (fragment_file.has_value() && !fragment_exists)
   {
      Log::Error() << "fragment shader file does not exist: " << fragment_file.value();
   }

   auto shader_loaded = false;
   if (vertex_exists && fragment_exists)
   {
      shader_loaded = instance->_shader.loadFromFile(vertex_file.value(), fragment_file.value());
   }
   else if (fragment_exists)
   {
      shader_loaded = instance->_shader.loadFromFragment(fragment_file.value());
   }

   if (!shader_loaded)
   {
      Log::Error() << "post processing mechanism has no usable shader, id: " << data._tmx_object->_name;
      return nullptr;
   }

   // every "u_" property becomes a uniform, so an arbitrary shader can be driven straight from tmx
   for (const auto& [key, property] : map)
   {
      if (!key.starts_with(uniform_prefix))
      {
         continue;
      }

      if (property->_value_type == "float")
      {
         instance->_uniforms.push_back({._name = key, ._value = property->_value_float.value_or(0.0f)});
      }
      else if (property->_value_type == "int")
      {
         instance->_uniforms.push_back({._name = key, ._value = property->_value_int.value_or(0)});
      }
      else if (property->_value_type == "bool")
      {
         instance->_uniforms.push_back({._name = key, ._value = property->_value_bool.value_or(false)});
      }
      else if (property->_value_string.has_value())
      {
         const auto& text = property->_value_string.value();
         const auto values = parseFloatList(text);

         if (values.size() == 2)
         {
            instance->_uniforms.push_back({._name = key, ._value = sf::Glsl::Vec2{values[0], values[1]}});
         }
         else if (values.size() == 3)
         {
            instance->_uniforms.push_back({._name = key, ._value = sf::Glsl::Vec3{values[0], values[1], values[2]}});
         }
         else if (values.size() == 4)
         {
            instance->_uniforms.push_back({._name = key, ._value = sf::Glsl::Vec4{values[0], values[1], values[2], values[3]}});
         }
         else if (values.size() == 1)
         {
            instance->_uniforms.push_back({._name = key, ._value = values[0]});
         }
         else if (std::filesystem::exists(text))
         {
            instance->_uniforms.push_back({._name = key, ._value = TexturePool::getInstance().get(text)});
         }
         else
         {
            Log::Error() << "post processing uniform '" << key << "' has no usable value: " << text;
         }
      }
   }

   Log::Info() << "post processing mechanism '" << data._tmx_object->_name << "' loaded " << fragment_file.value_or("") << " with "
               << instance->_uniforms.size() << " configured uniform(s)";

   return instance;
}
