#include "atmosphereshader.h"

#include "framework/tools/globalclock.h"
#include "framework/tools/log.h"
#include "game/io/texturepool.h"

void AtmosphereShader::initialize(const std::shared_ptr<sf::RenderTexture>& render_texture)
{
   _render_texture = render_texture;

   auto loaded = sf::Shader::loadFromFile({.fragmentPath = "data/shaders/water.frag"});
   if (!loaded.hasValue())
   {
      Log::Error() << "error loading water shader";
      return;
   }
   _shader = std::move(*loaded);

   _distortion_map = TexturePool::getInstance().get("data/effects/distortion_map.png");
   _distortion_map->setWrapMode(sf::TextureWrapMode::Repeat);
   _distortion_map->setSmooth(true);

   auto get_ul = [&](const char* name) -> std::optional<sf::Shader::UniformLocation>
   {
      const auto result = _shader->getUniformLocation(name);
      return result.hasValue() ? std::optional{*result} : std::nullopt;
   };
   _uniform_current_texture        = get_ul("current_texture");
   _uniform_distortion_map_texture = get_ul("distortion_map_texture");
   _uniform_physics_texture        = get_ul("physics_texture");
   _uniform_time                   = get_ul("time");
   _uniform_distortion_amplitude   = get_ul("distortion_amplitude");

   if (_uniform_current_texture.has_value())
   {
      _shader->setUniform(*_uniform_current_texture, sf::Shader::CurrentTexture);
   }
   if (_uniform_distortion_map_texture.has_value())
   {
      (void)_shader->setUniform(*_uniform_distortion_map_texture, *_distortion_map);
   }
   if (_uniform_physics_texture.has_value())
   {
      (void)_shader->setUniform(*_uniform_physics_texture, _render_texture->getTexture());
   }
}

void AtmosphereShader::update()
{
   if (!_shader.has_value())
   {
      return;
   }
   constexpr auto distortion_amplitude = 0.04f;
   constexpr auto time_factor = 1.0f;

   if (_uniform_time.has_value())
   {
      _shader->setUniform(*_uniform_time, GlobalClock::getInstance().getElapsedTimeInS() * time_factor);
   }
   if (_uniform_distortion_amplitude.has_value())
   {
      _shader->setUniform(*_uniform_distortion_amplitude, distortion_amplitude);
   }
}

const std::shared_ptr<sf::RenderTexture>& AtmosphereShader::getRenderTexture() const
{
   return _render_texture;
}

const sf::Shader& AtmosphereShader::getShader() const
{
   return *_shader;
}
