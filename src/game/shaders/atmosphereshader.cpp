#include "atmosphereshader.h"

#include "framework/tools/globalclock.h"
#include "framework/tools/log.h"
#include "game/io/texturepool.h"

AtmosphereShader::AtmosphereShader(uint32_t texture_width, uint32_t texture_height) : _render_texture(std::make_shared<sf::RenderTexture>())
{
   if (!_render_texture->create(static_cast<uint32_t>(texture_width), static_cast<uint32_t>(texture_height)))
   {
      Log::Fatal() << "failed to create texture";
   }
}

AtmosphereShader::~AtmosphereShader()
{
   _render_texture.reset();
}

void AtmosphereShader::initialize()
{
   if (!_shader.loadFromFile("data/shaders/water.frag", sf::Shader::Type::Fragment))
   {
      Log::Error() << "error loading water shader";
      return;
   }

   _distortion_map = TexturePool::getInstance().get("data/effects/distortion_map.png");
   _distortion_map->setRepeated(true);
   _distortion_map->setSmooth(true);

   _shader.setUniform("current_texture", sf::Shader::CurrentTexture);
   _shader.setUniform("distortion_map_texture", *_distortion_map);
   _shader.setUniform("physics_texture", _render_texture->getTexture());
}

void AtmosphereShader::update()
{
   constexpr auto distortion_amplitude = 0.04f;
   constexpr auto time_factor = 1.0f;

   _shader.setUniform("time", GlobalClock::getInstance().getElapsedTimeInS() * time_factor);
   _shader.setUniform("distortion_amplitude", distortion_amplitude);
}

const std::shared_ptr<sf::RenderTexture>& AtmosphereShader::getRenderTexture() const
{
   return _render_texture;
}

const sf::Shader& AtmosphereShader::getShader() const
{
   return _shader;
}
