#include "deathshader.h"

#include "player/player.h"

#include <iostream>


DeathShader::DeathShader(uint32_t width, uint32_t height)
{
   _render_texture = std::make_shared<sf::RenderTexture>();
   _render_texture->create(width, height);
}


DeathShader::~DeathShader()
{
   _render_texture.reset();
}



void DeathShader::initialize()
{
   if (!_shader.loadFromFile(
         "data/shaders/death.vert",
         "data/shaders/death.frag"
      )
   )
   {
      std::cout << "error loading shader" << std::endl;
      return;
   }

   if (!_flow_field_1.loadFromFile("data/effects/flowfield_1.png"))
   {
      std::cout << "error loading flowfield 1" << std::endl;
      return;
   }

   if (!_flow_field_2.loadFromFile("data/effects/flowfield_3.png"))
   {
      std::cout << "error loading flowfield 2" << std::endl;
      return;
   }

   _flow_field_1.setRepeated(true);
   _flow_field_1.setSmooth(true);
   _flow_field_2.setRepeated(true);
   _flow_field_2.setSmooth(true);

   _shader.setUniform("current_texture", sf::Shader::CurrentTexture);
   _shader.setUniform("flowfield_1", _flow_field_1);
   _shader.setUniform("flowfield_2", _flow_field_2);
}


void DeathShader::reset()
{
   _elapsed = 0.0f;
   _shader.setUniform("time", _elapsed);
}


void DeathShader::update(const sf::Time& dt)
{
   _elapsed += dt.asSeconds() * 0.5f;

   if (_elapsed > 1.0f)
   {
      _elapsed = 1.0f;
   }

   // for testing
   // mElapsed = fmod(mElapsed, 1.0f);

   // std::cout << mElapsed << std::endl;
   _shader.setUniform("time", _elapsed);
   _shader.setUniform(
      "flowfield_offset",
      Player::getCurrent()->isPointingLeft()
         ? sf::Glsl::Vec2(0.5f, -0.32f) // picked randomly
         : sf::Glsl::Vec2(0.8f, 0.8f)
   );
}


const sf::Shader& DeathShader::getShader() const
{
   return _shader;
}


const std::shared_ptr<sf::RenderTexture>& DeathShader::getRenderTexture() const
{
   return _render_texture;
}


