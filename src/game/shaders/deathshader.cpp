#include "deathshader.h"

#include "framework/tools/log.h"
#include "game/io/texturepool.h"
#include "game/player/playerregistry.h"

#include <iostream>

DeathShader::DeathShader(uint32_t width, uint32_t height)
{
   try
   {
      _render_texture = std::make_shared<sf::RenderTexture>(std::move(*sf::RenderTexture::create(sf::Vector2u{width, height})));
   }
   catch (...)
   {
      Log::Fatal() << "failed to create death shader texture" << std::endl;
   }
}

DeathShader::~DeathShader()
{
   _render_texture.reset();
}

void DeathShader::initialize()
{
   auto loaded = sf::Shader::loadFromFile({.vertexPath = "data/shaders/death.vert", .fragmentPath = "data/shaders/death.frag"});
   if (!loaded.hasValue())
   {
      Log::Error() << "error loading shader";
      return;
   }
   _shader = std::move(*loaded);

   _flow_field_1 = TexturePool::getInstance().get("data/effects/flowfield_1.png");
   _flow_field_2 = TexturePool::getInstance().get("data/effects/flowfield_3.png");

   _flow_field_1->setWrapMode(sf::TextureWrapMode::Repeat);
   _flow_field_1->setSmooth(true);
   _flow_field_2->setWrapMode(sf::TextureWrapMode::Repeat);
   _flow_field_2->setSmooth(true);

   auto get_ul = [&](const char* name) -> std::optional<sf::Shader::UniformLocation>
   {
      const auto result = _shader->getUniformLocation(name);
      return result.hasValue() ? std::optional{*result} : std::nullopt;
   };
   _uniform_current_texture  = get_ul("current_texture");
   _uniform_flowfield_1      = get_ul("flowfield_1");
   _uniform_flowfield_2      = get_ul("flowfield_2");
   _uniform_time             = get_ul("time");
   _uniform_flowfield_offset = get_ul("flowfield_offset");

   if (_uniform_current_texture.has_value())
   {
      _shader->setUniform(*_uniform_current_texture, sf::Shader::CurrentTexture);
   }
   if (_uniform_flowfield_1.has_value())
   {
      (void)_shader->setUniform(*_uniform_flowfield_1, *_flow_field_1);
   }
   if (_uniform_flowfield_2.has_value())
   {
      (void)_shader->setUniform(*_uniform_flowfield_2, *_flow_field_2);
   }
}

void DeathShader::reset()
{
   _elapsed = 0.0f;
   if (_shader.has_value() && _uniform_time.has_value())
   {
      _shader->setUniform(*_uniform_time, _elapsed);
   }
}

void DeathShader::update(const sf::Time& dt)
{
   _elapsed += dt.asSeconds() * 0.5f;

   if (_elapsed > 1.0f)
   {
      _elapsed = 1.0f;
   }

   if (!_shader.has_value())
   {
      return;
   }

   if (_uniform_time.has_value())
   {
      _shader->setUniform(*_uniform_time, _elapsed);
   }
   if (_uniform_flowfield_offset.has_value())
   {
      _shader->setUniform(
         *_uniform_flowfield_offset,
         PlayerRegistry::getFirst()->isPointingLeft() ? sf::Glsl::Vec2(0.5f, -0.32f)  // picked randomly
                                                      : sf::Glsl::Vec2(0.8f, 0.8f)
      );
   }
}

const sf::Shader& DeathShader::getShader() const
{
   return *_shader;
}

const std::shared_ptr<sf::RenderTexture>& DeathShader::getRenderTexture() const
{
   return _render_texture;
}
