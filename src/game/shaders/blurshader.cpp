#include "blurshader.h"

#include "framework/tools/log.h"

void BlurShader::initialize(
   const std::shared_ptr<sf::RenderTexture>& render_texture,
   const std::shared_ptr<sf::RenderTexture>& render_texture_scaled
)
{
   _render_texture = render_texture;
   _render_texture_scaled = render_texture_scaled;

   auto loaded = sf::Shader::loadFromFile({.fragmentPath = "data/shaders/blur.frag"});
   if (!loaded.hasValue())
   {
      Log::Error() << "error loading blur shader";
      return;
   }
   _shader = std::move(*loaded);

   _uniform_texture        = _shader->getUniformLocation("texture");
   _uniform_texture_width  = _shader->getUniformLocation("texture_width");
   _uniform_texture_height = _shader->getUniformLocation("texture_height");
   _uniform_blur_radius    = _shader->getUniformLocation("blur_radius");
   _uniform_add_factor     = _shader->getUniformLocation("add_factor");

   if (_uniform_texture.has_value())
   {
      (void)_shader->setUniform(*_uniform_texture, _render_texture->getTexture());
   }
}

void BlurShader::update()
{
   if (!_shader.has_value())
   {
      return;
   }
   // that implicitly scales the effect up by 2
   if (_uniform_texture_width.has_value())
   {
      _shader->setUniform(*_uniform_texture_width, 960 / 2);
   }
   if (_uniform_texture_height.has_value())
   {
      _shader->setUniform(*_uniform_texture_height, 540 / 2);
   }
   if (_uniform_blur_radius.has_value())
   {
      _shader->setUniform(*_uniform_blur_radius, 20.0f);
   }
   if (_uniform_add_factor.has_value())
   {
      _shader->setUniform(*_uniform_add_factor, 1.0f);
   }
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
   return *_shader;
}
