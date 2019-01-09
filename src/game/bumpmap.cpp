#include "bumpmap.h"



BumpMap::BumpMap()
{
}



void BumpMap::initialize()
{
   if (sf::Shader::isAvailable())
   {
      // load both shaders
      if (mShader.loadFromFile(
            "data/shaders/bumpmap_vert.glsl",
            "data/shaders/bumpmap_frag.glsl"
         )
      )
      {
         printf("loaded shader: %d\n", mShader.getNativeHandle());
         fflush(stdout);

         mLightPosition.x = 0.5f;
         mLightPosition.y = 1.0f;
         mLightPosition.z = 0.0f;
      }
      else
      {
         printf("error loading shader\n");
         fflush(stdout);
      }
   }
   else
   {
      printf("shader's not available\n");
      fflush(stdout);
   }
}


//void BumpMap::bind()
//{
//   sf::Shader::bind(&mShader);
//}


//void BumpMap::unbind()
//{
//   sf::Shader::bind(0);
//}


//sf::Texture *BumpMap::getTexture() const
//{
//   return mTexture;
//}


//void BumpMap::setTexture(sf::Texture *texture)
//{
//   mTexture = texture;
//}


//sf::Texture *BumpMap::getNormalMap() const
//{
//   return mNormalMap;
//}


//void BumpMap::setNormalMap(sf::Texture *normalMap)
//{
//   mNormalMap = normalMap;
//}


sf::Shader* BumpMap::getShader()
{
   return &mShader;
}


const sf::Vector3f &BumpMap::getLightPosition() const
{
    return mLightPosition;
}


void BumpMap::setLightPosition(const sf::Vector3f &pos)
{
    mLightPosition = pos;
}





