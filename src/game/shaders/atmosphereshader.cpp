#include "atmosphereshader.h"

#include "framework/tools/globalclock.h"
#include "framework/tools/log.h"

#include <iostream>

//----------------------------------------------------------------------------------------------------------------------
AtmosphereShader::AtmosphereShader(uint32_t texture_width, uint32_t texture_height) : _render_texture(std::make_shared<sf::RenderTexture>())
{
   if (!_render_texture->create(static_cast<uint32_t>(texture_width), static_cast<uint32_t>(texture_height)))
   {
      Log::Fatal() << "failed to create texture";
   }
}

//----------------------------------------------------------------------------------------------------------------------
AtmosphereShader::~AtmosphereShader()
{
   _render_texture.reset();
}

//----------------------------------------------------------------------------------------------------------------------
void AtmosphereShader::initialize()
{
   if (!_shader.loadFromFile("data/shaders/water.frag", sf::Shader::Fragment))
   {
      Log::Error() << "error loading water shader";
      return;
   }

   if (!_distortion_map.loadFromFile("data/effects/distortion_map.png"))
   {
      Log::Error() << "error loading distortion map";
      return;
   }

   _distortion_map.setRepeated(true);
   _distortion_map.setSmooth(true);

   _shader.setUniform("currentTexture", sf::Shader::CurrentTexture);
   _shader.setUniform("distortionMapTexture", _distortion_map);
   _shader.setUniform("physicsTexture", _render_texture->getTexture());
}

//----------------------------------------------------------------------------------------------------------------------
void AtmosphereShader::update()
{
   float distortionFactor = 0.02f;

   _shader.setUniform("time", GlobalClock::getInstance().getElapsedTimeInS() * 0.2f);
   _shader.setUniform("distortionFactor", distortionFactor);
}

//----------------------------------------------------------------------------------------------------------------------
const std::shared_ptr<sf::RenderTexture>& AtmosphereShader::getRenderTexture() const
{
   return _render_texture;
}

//----------------------------------------------------------------------------------------------------------------------
const sf::Shader& AtmosphereShader::getShader() const
{
   return _shader;
}
