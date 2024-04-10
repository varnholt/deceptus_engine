#pragma once

#include <SFML/Graphics.hpp>
#include <memory>

class DeathShader
{
   public:

      DeathShader(
         uint32_t width,
         uint32_t height
      );
      virtual ~DeathShader();

      void initialize();
      void reset();
      void update(const sf::Time& dt);

      const sf::Shader& getShader() const;
      const std::shared_ptr<sf::RenderTexture>& getRenderTexture() const;


   private:

      sf::Shader _shader;

      std::shared_ptr<sf::RenderTexture> _render_texture;

      std::shared_ptr<sf::Texture> _flow_field_1;
      std::shared_ptr<sf::Texture> _flow_field_2;

      float _elapsed = 0.0f;
};
