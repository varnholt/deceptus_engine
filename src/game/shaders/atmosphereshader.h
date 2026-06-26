#pragma once

#include <SFML/Graphics.hpp>
#include <memory>
#include <optional>

/// \brief wraps the water post-process shader used for atmosphere distortion.
class AtmosphereShader
{
public:
   AtmosphereShader() = default;

   /// \brief loads shader resources and binds textures used by the effect.
   /// \param render_texture render texture whose texture is bound as physics_texture.
   void initialize(const std::shared_ptr<sf::RenderTexture>& render_texture);

   /// \brief updates time-based uniforms for animated distortion.
   void update();

   /// \brief returns the render texture bound during initialization.
   /// \return shared pointer reference to the shader input render texture.
   const std::shared_ptr<sf::RenderTexture>& getRenderTexture() const;

   /// \brief returns the configured fragment shader.
   /// \return const reference to the underlying SFML shader object.
   const sf::Shader& getShader() const;

private:
   std::shared_ptr<sf::RenderTexture> _render_texture;
   std::shared_ptr<sf::Texture> _distortion_map;
   std::optional<sf::Shader> _shader;
};
