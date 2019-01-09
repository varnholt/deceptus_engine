#ifndef LEVEL_H
#define LEVEL_H

// std
#include <list>
#include <map>

// sfml
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Vector3.hpp>

// box2d
#include "box2d/Box2D.h"


// forward declarations
class BumpMap;
class BulletPlayer;


class BulletLevel
{
   public:

      enum TextureType {
         TextureTypeColor,
         TextureTypeDisplacement,
         TextureTypeNormal,
         TextureTypeOcclusion,
         TextureTypeSpecular
      };

      BulletLevel();

      virtual ~BulletLevel();

      virtual void initialize();

      // setter for start position
      sf::Vector3f getStartPosition() const;
      void setStartPosition(const sf::Vector3f &pos);

      // level size
      sf::Vector2u getSize() const;
      void setSize(const sf::Vector2u &size);

      sf::Sprite* getLevelSprite();

      BumpMap* getBumpMap();


      // add sprites
      void addSprite(sf::Sprite*);


      b2World *getWorld() const;
      void setWorld(b2World *world);


      void parsePsdPaths(
         std::map<b2Body*, b2Vec2*> &pointMap,
         std::map<b2Body*, int> &pointSizeMap
      );


      void generateSprites(const sf::Image &source, TextureType textureType);
      void draw(sf::RenderTarget& target) const;


protected:

      void createLevelBoundaries();


      sf::Texture* mBackgroundTexture;
      sf::Sprite* mSprite;

      std::list<sf::Sprite*> mSprites;


      sf::Vector3f mStartPosition;
      sf::Vector2u mSize;

      // box2d world
      b2World* mWorld;

      BumpMap* mBumpMap;


      std::vector<sf::Sprite*> mLevelColorSprites;
      std::vector<sf::Texture*> mLevelColorTextures;

      std::vector<sf::Sprite*> mLevelNormalSprites;
      std::vector<sf::Texture*> mLevelNormalTextures;
};

#endif // LEVEL_H
