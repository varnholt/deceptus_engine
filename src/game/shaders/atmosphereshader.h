#pragma once

#include <SFML/Graphics.hpp>
#include <memory>

class AtmosphereShader
{
public:
   AtmosphereShader() = default;

   void initialize(const std::shared_ptr<sf::RenderTexture>& render_texture);
   void update();

   const std::shared_ptr<sf::RenderTexture>& getRenderTexture() const;
   const sf::Shader& getShader() const;

private:
   std::shared_ptr<sf::RenderTexture> _render_texture;
   std::shared_ptr<sf::Texture> _distortion_map;
   sf::Shader _shader;
};
