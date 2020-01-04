#include "blurshader.h"

#include <iostream>


//----------------------------------------------------------------------------------------------------------------------
BlurShader::BlurShader(
   uint32_t width,
   uint32_t height
)
{
   mRenderTexture = std::make_shared<sf::RenderTexture>();
   mRenderTexture->create(width, height);
   mRenderTextureScaled = std::make_shared<sf::RenderTexture>();
   mRenderTextureScaled->create(960, 540);
   mRenderTextureScaled->setSmooth(true);
}


//----------------------------------------------------------------------------------------------------------------------
BlurShader::~BlurShader()
{
   mRenderTexture.reset();
   mRenderTextureScaled.reset();
}


//----------------------------------------------------------------------------------------------------------------------
void BlurShader::initialize()
{
   if (!mShader.loadFromFile("data/shaders/blur.frag", sf::Shader::Fragment))
   {
      std::cout << "error loading blur shader" << std::endl;
      return;
   }

   mShader.setUniform("texture", mRenderTexture->getTexture());
}


//----------------------------------------------------------------------------------------------------------------------
void BlurShader::update()
{
   // that implicitly scales the effect up by 2
   mShader.setUniform("texture_width", 960/2);
   mShader.setUniform("texture_height", 540/2);

   mShader.setUniform("blur_radius", 20.0f);
   mShader.setUniform("add_factor", 1.0f);
}


//----------------------------------------------------------------------------------------------------------------------
void BlurShader::clearTexture()
{
   mRenderTexture->clear({0, 0, 0, 0});
}


//----------------------------------------------------------------------------------------------------------------------
const std::shared_ptr<sf::RenderTexture>& BlurShader::getRenderTexture() const
{
   return mRenderTexture;
}


//----------------------------------------------------------------------------------------------------------------------
const std::shared_ptr<sf::RenderTexture>& BlurShader::getRenderTextureScaled() const
{
   return mRenderTextureScaled;
}

const sf::Shader& BlurShader::getShader() const
{
   return mShader;
}
