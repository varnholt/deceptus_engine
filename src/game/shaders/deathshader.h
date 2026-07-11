#pragma once

#include <SFML/Graphics.hpp>
#include <memory>
#ifdef __EMSCRIPTEN__
#include <optional>
#endif

/// \brief manages the death screen shader and its flow-field resources.
class DeathShader
{
public:
   /// \brief creates the internal render texture used by the death effect.
   /// \param width render texture width in pixels.
   /// \param height render texture height in pixels.
   DeathShader(uint32_t width, uint32_t height);

   /// \brief releases the owned render texture.
   virtual ~DeathShader();

   /// \brief loads shader files, flow-field textures, and static uniforms.
   void initialize();

   /// \brief resets effect progression and updates the shader time uniform to zero.
   void reset();

   /// \brief advances the effect timer, clamps it, and updates directional flow offset.
   /// \param dt frame delta time used to advance the normalized death effect time.
   void update(const sf::Time& dt);

   /// \brief returns the configured death shader.
   /// \return const reference to the underlying SFML shader.
   const sf::Shader& getShader() const;

   /// \brief returns the render texture used for death pass rendering.
   /// \return shared pointer reference to the death render texture.
   const std::shared_ptr<sf::RenderTexture>& getRenderTexture() const;

private:
#ifdef __EMSCRIPTEN__
   std::optional<sf::Shader> _shader;
#else
   sf::Shader _shader;
#endif

   std::shared_ptr<sf::RenderTexture> _render_texture;

   std::shared_ptr<sf::Texture> _flow_field_1;
   std::shared_ptr<sf::Texture> _flow_field_2;

#ifdef __EMSCRIPTEN__
   std::optional<sf::Shader::UniformLocation> _uniform_current_texture;
   std::optional<sf::Shader::UniformLocation> _uniform_flowfield_1;
   std::optional<sf::Shader::UniformLocation> _uniform_flowfield_2;
   std::optional<sf::Shader::UniformLocation> _uniform_time;
   std::optional<sf::Shader::UniformLocation> _uniform_flowfield_offset;
#endif

   float _elapsed = 0.0f;
};
