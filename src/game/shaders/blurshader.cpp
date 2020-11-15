#include "blurshader.h"

#include <iostream>

#define SUPPORT_STENCIL_BITS 1


//----------------------------------------------------------------------------------------------------------------------
BlurShader::BlurShader(
   uint32_t width,
   uint32_t height
)
{
#ifdef SUPPORT_STENCIL_BITS
   sf::ContextSettings contextSettings;
   contextSettings.stencilBits = 8;
#endif

   mRenderTexture = std::make_shared<sf::RenderTexture>();

#ifdef SUPPORT_STENCIL_BITS
   mRenderTexture->create(width, height, contextSettings);
#else
   mRenderTexture->create(width, height);
#endif

   mRenderTextureScaled = std::make_shared<sf::RenderTexture>();

#ifdef SUPPORT_STENCIL_BITS
   mRenderTextureScaled->create(960, 540, contextSettings);
#else
   mRenderTextureScaled->create(960, 540);
#endif
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
