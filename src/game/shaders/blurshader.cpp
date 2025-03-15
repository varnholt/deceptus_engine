#include "blurshader.h"

#include "framework/tools/log.h"

#include <iostream>

BlurShader::BlurShader(uint32_t width, uint32_t height)
{
   sf::ContextSettings contextSettings;
   contextSettings.stencilBits = 8;

   try
   {
      _render_texture = std::make_shared<sf::RenderTexture>(sf::Vector2u{width, height}, contextSettings);
   }
   catch (...)
   {
      Log::Fatal() << "failed to create render texture";
   }

   try
   {
      _render_texture_scaled = std::make_shared<sf::RenderTexture>(sf::Vector2u{960, 540}, contextSettings);
   }
   catch (...)
   {
      Log::Fatal() << "failed to create scaled texture";
   }

   _render_texture_scaled->setSmooth(true);
}

BlurShader::~BlurShader()
{
   _render_texture.reset();
   _render_texture_scaled.reset();
}

void BlurShader::initialize()
{
   if (!_shader.loadFromFile("data/shaders/blur.frag", sf::Shader::Type::Fragment))
   {
      Log::Error() << "error loading blur shader";
      return;
   }

   _shader.setUniform("texture", _render_texture->getTexture());
}

void BlurShader::update()
{
   // that implicitly scales the effect up by 2
   _shader.setUniform("texture_width", 960 / 2);
   _shader.setUniform("texture_height", 540 / 2);

   _shader.setUniform("blur_radius", 20.0f);
   _shader.setUniform("add_factor", 1.0f);
}

void BlurShader::clearTexture()
{
   _render_texture->clear({0, 0, 0, 0});
}

const std::shared_ptr<sf::RenderTexture>& BlurShader::getRenderTexture() const
{
   return _render_texture;
}

const std::shared_ptr<sf::RenderTexture>& BlurShader::getRenderTextureScaled() const
{
   return _render_texture_scaled;
}

const sf::Shader& BlurShader::getShader() const
{
   return _shader;
}
