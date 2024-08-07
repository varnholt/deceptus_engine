#include "deathshader.h"

#include "framework/tools/log.h"
#include "game/io/texturepool.h"
#include "game/player/player.h"

#include <iostream>


DeathShader::DeathShader(uint32_t width, uint32_t height)
 : _render_texture(std::make_shared<sf::RenderTexture>())
{
   if (!_render_texture->create(width, height))
   {
      Log::Fatal() << "failed to create death shader texture" << std::endl;
   }
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
      Log::Error() << "error loading shader";
      return;
   }

   _flow_field_1 = TexturePool::getInstance().get("data/effects/flowfield_1.png");
   _flow_field_2 = TexturePool::getInstance().get("data/effects/flowfield_3.png");

   _flow_field_1->setRepeated(true);
   _flow_field_1->setSmooth(true);
   _flow_field_2->setRepeated(true);
   _flow_field_2->setSmooth(true);

   _shader.setUniform("current_texture", sf::Shader::CurrentTexture);
   _shader.setUniform("flowfield_1", *_flow_field_1);
   _shader.setUniform("flowfield_2", *_flow_field_2);
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


