#pragma once

#include <SFML/Graphics.hpp>
#include <memory>

class BlurShader
{
   public:
      BlurShader(
         uint32_t width,
         uint32_t height
      );

      ~BlurShader();

      void initialize();
      void update();
      void clearTexture();

      const std::shared_ptr<sf::RenderTexture>& getRenderTexture() const;
      const std::shared_ptr<sf::RenderTexture>& getRenderTextureScaled() const;

      const sf::Shader& getShader() const;

   private:
      sf::Shader _shader;
      std::shared_ptr<sf::RenderTexture> _render_texture;
      std::shared_ptr<sf::RenderTexture> _render_texture_scaled;
};

