#pragma once

#include <SFML/Graphics.hpp>
#include <memory>

class AtmosphereShader
{
public:
   AtmosphereShader(uint32_t texture_width, uint32_t texture_height);

   ~AtmosphereShader();

   void initialize();
   void update();

   const std::shared_ptr<sf::RenderTexture>& getRenderTexture() const;
   const sf::Shader& getShader() const;

private:
   std::shared_ptr<sf::RenderTexture> _render_texture;

   sf::Shader _shader;
   sf::Texture _distortion_map;
};
