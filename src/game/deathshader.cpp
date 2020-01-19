#include "deathshader.h"

#include "player.h"

#include <iostream>


DeathShader::DeathShader(uint32_t width, uint32_t height)
{
   mRenderTexture = std::make_shared<sf::RenderTexture>();
   mRenderTexture->create(width, height);
}


DeathShader::~DeathShader()
{
   mRenderTexture.reset();
}



void DeathShader::initialize()
{
   if (!mShader.loadFromFile(
         "data/shaders/death.vert",
         "data/shaders/death.frag"
      )
   )
   {
      std::cout << "error loading shader" << std::endl;
      return;
   }

   if (!mFlowField1.loadFromFile("data/effects/flowfield_1.png"))
   {
      std::cout << "error loading flowfield 1" << std::endl;
      return;
   }

   if (!mFlowField2.loadFromFile("data/effects/flowfield_3.png"))
   {
      std::cout << "error loading flowfield 2" << std::endl;
      return;
   }

   mFlowField1.setRepeated(true);
   mFlowField1.setSmooth(true);
   mFlowField2.setRepeated(true);
   mFlowField2.setSmooth(true);

   mShader.setUniform("current_texture", sf::Shader::CurrentTexture);
   mShader.setUniform("flowfield_1", mFlowField1);
   mShader.setUniform("flowfield_2", mFlowField2);
}


void DeathShader::reset()
{
   mElapsed = 0.0f;
}


void DeathShader::update(const sf::Time& dt)
{
   mElapsed += dt.asSeconds() * 0.5f;

   if (mElapsed > 1.0f)
   {
      mElapsed = 1.0f;
   }

   // for testing
   // mElapsed = fmod(mElapsed, 1.0f);

   // std::cout << mElapsed << std::endl;
   mShader.setUniform("time", mElapsed);
   mShader.setUniform(
      "flowfield_offset",
      Player::getCurrent()->isPointingLeft()
         ? sf::Glsl::Vec2(0.5f, -0.32f) // picked randomly
         : sf::Glsl::Vec2(0.8f, 0.8f)
   );
}


const sf::Shader& DeathShader::getShader() const
{
   return mShader;
}


const std::shared_ptr<sf::RenderTexture>& DeathShader::getRenderTexture() const
{
   return mRenderTexture;
}


