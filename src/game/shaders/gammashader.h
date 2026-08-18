#pragma once

#include "framework/tools/sfmlshader.h"

#include <SFML/Graphics.hpp>

/// \brief wraps the brightness shader used for gamma-style screen correction.
class GammaShader
{
public:
   /// \brief constructs an uninitialized gamma shader wrapper.
   GammaShader() = default;

   /// \brief loads the brightness fragment shader from disk.
   void initialize();

   /// \brief updates the gamma uniform from the current brightness configuration.
   void update();

   /// \brief binds the input texture sampled by the shader.
   /// \param texture source texture passed to the shader as texture uniform.
   void setTexture(const sf::Texture& texture);

   /// \brief returns the configured gamma shader.
   /// \return const reference to the underlying SFML shader.
   const sf::Shader& getGammaShader() const;

private:
   sfcompat::Shader _gamma_shader;
};
