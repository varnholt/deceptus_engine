#include "deathshader.h"

#include <iostream>


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

   //   mFlowField1.setRepeated(true);
   //   mFlowField1.setSmooth(true);
   //   mFlowField2.setRepeated(true);
   //   mFlowField2.setSmooth(true);

   mShader.setUniform("current_texture", sf::Shader::CurrentTexture);
   mShader.setUniform("flowfield_1", mFlowField1);
   mShader.setUniform("flowfield_2", mFlowField2);
}


void DeathShader::update(const sf::Time& dt)
{
   mElapsed += fmod(dt.asSeconds() * 0.01f, 1.0f);

   // std::cout << mElapsed << std::endl;
   mShader.setUniform("time", mElapsed);
}


const sf::Shader& DeathShader::getShader() const
{
   return mShader;
}


