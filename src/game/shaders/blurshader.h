#pragma once

#include <SFML/Graphics.hpp>
#include <memory>
#ifdef __EMSCRIPTEN__
#include <optional>
#endif

/// \brief wraps the full-screen blur fragment shader used for post processing.
class BlurShader
{
public:
   /// \brief constructs an uninitialized blur shader wrapper.
   BlurShader() = default;

   /// \brief stores render textures, loads blur.frag, and binds the source texture uniform.
   /// \param render_texture full-resolution source texture used by the shader.
   /// \param render_texture_scaled scaled render texture used by the blur pass pipeline.
   void
   initialize(const std::shared_ptr<sf::RenderTexture>& render_texture, const std::shared_ptr<sf::RenderTexture>& render_texture_scaled);

   /// \brief updates blur uniforms such as sample dimensions, radius, and blend factor.
   void update();

   /// \brief clears the main render texture to transparent black.
   void clearTexture();

   /// \brief returns the full-resolution render texture used by the blur shader.
   /// \return shared pointer reference to the source render texture.
   const std::shared_ptr<sf::RenderTexture>& getRenderTexture() const;

   /// \brief returns the scaled render texture used in the blur pipeline.
   /// \return shared pointer reference to the scaled render texture.
   const std::shared_ptr<sf::RenderTexture>& getRenderTextureScaled() const;

   /// \brief returns the configured blur shader instance.
   /// \return const reference to the underlying SFML shader.
   const sf::Shader& getShader() const;

private:
#ifdef __EMSCRIPTEN__
   std::optional<sf::Shader> _shader;
#else
   sf::Shader _shader;
#endif
   std::shared_ptr<sf::RenderTexture> _render_texture;
   std::shared_ptr<sf::RenderTexture> _render_texture_scaled;
#ifdef __EMSCRIPTEN__
   std::optional<sf::Shader::UniformLocation> _uniform_texture;
   std::optional<sf::Shader::UniformLocation> _uniform_texture_width;
   std::optional<sf::Shader::UniformLocation> _uniform_texture_height;
   std::optional<sf::Shader::UniformLocation> _uniform_blur_radius;
   std::optional<sf::Shader::UniformLocation> _uniform_add_factor;
#endif
};
