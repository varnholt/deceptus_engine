#ifndef BUMPMAP_H
#define BUMPMAP_H


// sfml
#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/Texture.hpp>


class BumpMap
{

public:

   BumpMap();


   void initialize();

//   void bind();
//   void unbind();


//   sf::Texture *getTexture() const;
//   void setTexture(sf::Texture *tex);


//   sf::Texture *getNormalMap() const;
//   void setNormalMap(sf::Texture *map);


   sf::Shader* getShader();



   const sf::Vector3f& getLightPosition() const;
   void setLightPosition(const sf::Vector3f &pos);


protected:

   sf::Shader mShader;

   sf::Vector3f mLightPosition;

   //   sf::Texture* mTexture;
//   sf::Texture* mNormalMap;

};

#endif // BUMPMAP_H
