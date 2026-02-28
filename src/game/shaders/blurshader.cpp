#include "blurshader.h"

#include "framework/tools/log.h"

#include <iostream>

void BlurShader::initialize(const std::shared_ptr<sf::RenderTexture>& render_texture, const std::shared_ptr<sf::RenderTexture>& render_texture_scaled)
{
   _render_texture = render_texture;
   _render_texture_scaled = render_texture_scaled;

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
