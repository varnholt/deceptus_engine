#include "blurshader.h"

#include "framework/tools/log.h"

void BlurShader::initialize(
   const std::shared_ptr<sf::RenderTexture>& render_texture,
   const std::shared_ptr<sf::RenderTexture>& render_texture_scaled
)
{
   _render_texture = render_texture;
   _render_texture_scaled = render_texture_scaled;

#ifdef __EMSCRIPTEN__
   auto loaded = sf::Shader::loadFromFile({.fragmentPath = "data/shaders/blur.frag"});
   if (!loaded.hasValue())
   {
      Log::Error() << "error loading blur shader";
      return;
   }
   _shader = std::move(*loaded);

   auto get_ul = [&](const char* name) -> std::optional<sf::Shader::UniformLocation>
   {
      const auto result = _shader->getUniformLocation(name);
      return result.hasValue() ? std::optional{*result} : std::nullopt;
   };
   _uniform_texture = get_ul("u_texture");
   _uniform_texture_width = get_ul("texture_width");
   _uniform_texture_height = get_ul("texture_height");
   _uniform_blur_radius = get_ul("blur_radius");
   _uniform_add_factor = get_ul("add_factor");

   if (_uniform_texture.has_value())
   {
      (void)_shader->setUniform(*_uniform_texture, _render_texture->getTexture());
   }
#else
   if (!_shader.loadFromFile("data/shaders/blur.frag", sf::Shader::Type::Fragment))
   {
      Log::Error() << "error loading blur shader";
      return;
   }

   _shader.setUniform("texture", _render_texture->getTexture());
#endif
}

void BlurShader::update()
{
#ifdef __EMSCRIPTEN__
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
#else
   // that implicitly scales the effect up by 2
   _shader.setUniform("texture_width", 960 / 2);
   _shader.setUniform("texture_height", 540 / 2);

   _shader.setUniform("blur_radius", 20.0f);
   _shader.setUniform("add_factor", 1.0f);
#endif
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
#ifdef __EMSCRIPTEN__
   return *_shader;
#else
   return _shader;
#endif
}
