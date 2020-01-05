#include "deathshader.h"

#include <iostream>


DeathShader::DeathShader()
{
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

    mShader.setUniform("texture", mRenderTexture->getTexture());
}


void DeathShader::update()
{
   // that implicitly scales the effect up by 2
   mShader.setUniform("texture_width", 960/2);
   mShader.setUniform("texture_height", 540/2);

   mShader.setUniform("blur_radius", 20.0f);
   mShader.setUniform("add_factor", 1.0f);
}
