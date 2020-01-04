#include "atmosphereshader.h"

#include "globalclock.h"

#include <iostream>


//----------------------------------------------------------------------------------------------------------------------
AtmosphereShader::AtmosphereShader(
   uint32_t textureWidth,
   uint32_t textureHeight
)
{
   mRenderTexture = std::make_shared<sf::RenderTexture>();
   mRenderTexture->create(
      static_cast<uint32_t>(textureWidth),
      static_cast<uint32_t>(textureHeight)
   );
}


//----------------------------------------------------------------------------------------------------------------------
AtmosphereShader::~AtmosphereShader()
{
   mRenderTexture.reset();
}


//----------------------------------------------------------------------------------------------------------------------
void AtmosphereShader::initialize()
{
   if (!mShader.loadFromFile("data/shaders/water.frag", sf::Shader::Fragment))
   {
      std::cout << "error loading water shader" << std::endl;
      return;
   }

   if (!mDistortionMap.loadFromFile("data/effects/distortion_map.png"))
   {
      std::cout << "error loading distortion map" << std::endl;
      return;
   }

   mDistortionMap.setRepeated(true);
   mDistortionMap.setSmooth(true);

   mShader.setUniform("currentTexture", sf::Shader::CurrentTexture);
   mShader.setUniform("distortionMapTexture", mDistortionMap);
   mShader.setUniform("physicsTexture", mRenderTexture->getTexture());
}


//----------------------------------------------------------------------------------------------------------------------
void AtmosphereShader::update()
{
  float distortionFactor = 0.02f;

  mShader.setUniform("time", GlobalClock::getInstance()->getElapsedTimeInS() * 0.2f);
  mShader.setUniform("distortionFactor", distortionFactor);
}


//----------------------------------------------------------------------------------------------------------------------
const std::shared_ptr<sf::RenderTexture>& AtmosphereShader::getRenderTexture() const
{
   return mRenderTexture;
}


//----------------------------------------------------------------------------------------------------------------------
const sf::Shader& AtmosphereShader::getShader() const
{
   return mShader;
}

