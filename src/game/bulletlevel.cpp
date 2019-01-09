// base
#include "bulletlevel.h"

// box2d
#include <box2d/Box2D.h>

// std
#include <vector>

// game
#include "game/bumpmap.h"
#include "game/constants.h"

// image
#include "image/psd.h"

// poly2tri
#include "poly2tri/poly2tri.h"
#include "poly2tri/common/shapes.h"

// sfml
#include <SFML/Graphics/RenderTarget.hpp>


BulletLevel::BulletLevel()
 : mBackgroundTexture(0),
   mSprite(0),
   mWorld(0),
   mBumpMap(0)
{
   mBumpMap = new BumpMap();
}


BulletLevel::~BulletLevel()
{
   delete mBackgroundTexture;
   mBackgroundTexture = 0;

   delete mSprite;
   mSprite = 0;

   delete mWorld;
   mWorld = 0;

   delete mBumpMap;
   mBumpMap = 0;
}


void BulletLevel::initialize()
{
   mBumpMap->initialize();
}


void BulletLevel::addSprite(sf::Sprite * sprite)
{
   mSprites.push_back(sprite);
}


b2World *BulletLevel::getWorld() const
{
   return mWorld;
}


void BulletLevel::setWorld(b2World *world)
{
   mWorld = world;
}



sf::Vector2u BulletLevel::getSize() const
{
   return mSize;
}


void BulletLevel::setSize(const sf::Vector2u &levelSize)
{
   mSize = levelSize;
}


sf::Sprite* BulletLevel::getLevelSprite()
{
   return mSprite;
}


BumpMap *BulletLevel::getBumpMap()
{
   return mBumpMap;
}


sf::Vector3f BulletLevel::getStartPosition() const
{
   return mStartPosition;
}


void BulletLevel::setStartPosition(const sf::Vector3f &pos)
{
   mStartPosition = pos;
}



void BulletLevel::createLevelBoundaries()
{
   int levelWidth = 0;
   int levelHeight = 0;

   PSD psd;

   psd.load("data/level_big_paths_2.psd");

   int layerCount = psd.getLayerCount();

   if (layerCount > 0)
   {
      levelWidth = psd.getLayer(0)->getWidth();
      levelHeight = psd.getLayer(0)->getHeight();

      setSize(sf::Vector2u(levelWidth, levelHeight));
   };

   b2BodyDef levelBoundsBodyDef;
   levelBoundsBodyDef.type = b2_staticBody;
   levelBoundsBodyDef.position.Set(0, 0);

   b2Body* levelBoundsBody = mWorld->CreateBody(&levelBoundsBodyDef);
   b2PolygonShape levelBoundsShape;
   b2FixtureDef levelBoundsFixtureDef;

   levelBoundsFixtureDef.shape = &levelBoundsShape;
   levelBoundsFixtureDef.density = 1;

//   levelBoundsShape.SetAsBox( 20, 1, b2Vec2(0, 0), 0);//ground
//   levelBoundsBody->CreateFixture(&levelBoundsFixtureDef);

   // ceiling
   levelBoundsShape.SetAsBox(
      levelWidth * MPP * 0.5f,
      0.1f,
      b2Vec2(
         levelWidth * MPP * 0.5f,
         0
      ),
      0
   );

   levelBoundsBody->CreateFixture(&levelBoundsFixtureDef);

   // left wall
   levelBoundsShape.SetAsBox(
      0.1f,
      levelHeight * MPP * 0.5f,
      b2Vec2(
         0,
         levelHeight * MPP * 0.5f
      ),
      0
   );

   levelBoundsBody->CreateFixture(&levelBoundsFixtureDef);

   // right wall
   levelBoundsShape.SetAsBox(
      0.1f,
      levelHeight * MPP * 0.5f,
      b2Vec2(
         levelWidth * MPP,
         levelHeight * MPP * 0.5f
      ),
      0
   );

   levelBoundsBody->CreateFixture(&levelBoundsFixtureDef);
}


void BulletLevel::parsePsdPaths(
   std::map<b2Body*, b2Vec2*>& pointMap,
   std::map<b2Body*, int>& pointSizeMap
)
{
   PSD psd;
   psd.load("data/level_big_paths_2.psd");

   int pathCount = psd.getPathCount();

   printf("paths found: %d\n", pathCount);
   fflush(stdout);

   for (int pathIndex = 0; pathIndex < pathCount; pathIndex++)
   {
      PSD::Path* path = psd.getPath(pathIndex);

      int positionCount = path->getPositionCount();

      PSD::Path::Position pos;
      b2Vec2* points = new b2Vec2[positionCount];

      std::vector<b2Vec2>* positionVector = new std::vector<b2Vec2>();

      std::vector<p2t::Point*> polyLine;

      for (int positionIndex = 0; positionIndex < positionCount; positionIndex++)
      {
         pos = path->getPosition(positionIndex);

         points[positionIndex].x = pos.x / PPM;
         points[positionIndex].y = pos.y / PPM;

         positionVector->push_back(b2Vec2(pos.x, pos.y));

         printf(
            "path: %d, pos: %d, x: %f, y: %f\n",
            pathIndex,
            positionIndex,
            pos.x / PPM,
            pos.y / PPM
         );

         fflush(stdout);

         p2t::Point* p = new p2t::Point(pos.x/PPM, pos.y/PPM);
         polyLine.push_back(p);
      }

      p2t::CDT cdt(polyLine);
      cdt.Triangulate();

      std::vector<p2t::Triangle*> triangles = cdt.GetTriangles();


      b2BodyDef bodyDef;
      bodyDef.position.Set(
         0,
         0
      );

      bodyDef.type = b2_staticBody;
      b2Body* body = mWorld->CreateBody(&bodyDef);

      pointMap[body]=points;
      pointSizeMap[body]=positionCount;

      for (int i = 0; i < triangles.size(); i++)
      {
         p2t::Point* a = triangles[i]->GetPoint(0);
         p2t::Point* b = triangles[i]->GetPoint(1);
         p2t::Point* c = triangles[i]->GetPoint(2);

         printf(
            "tri %d: %f,%f | %f,%f | %f,%f\n",
            i,
            a->x, a->y, b->x, b->y, c->x, c->y
         );

         fflush(stdout);

         b2Vec2* trianglePoints = new b2Vec2[3];
         trianglePoints[0].x = a->x;
         trianglePoints[0].y = a->y;
         trianglePoints[1].x = b->x;
         trianglePoints[1].y = b->y;
         trianglePoints[2].x = c->x;
         trianglePoints[2].y = c->y;

         b2PolygonShape polygonShape;
         polygonShape.Set(trianglePoints, 3);

         b2FixtureDef fixtureDef;
         fixtureDef.density = 0.f;
         fixtureDef.shape = &polygonShape;

         body->CreateFixture(&fixtureDef);
      }
   }
}




void BulletLevel::generateSprites(
   const sf::Image& source,
   TextureType textureType
)
{
   unsigned int size = 512;

   std::string typeStr;

   std::vector<sf::Sprite*>* spriteList = 0;
   std::vector<sf::Texture*>* textureList = 0;
   switch (textureType)
   {
      case BulletLevel::TextureTypeColor:
         spriteList = &mLevelColorSprites;
         textureList = &mLevelColorTextures;
         typeStr = "colors";
         break;
      case BulletLevel::TextureTypeNormal:
         spriteList = &mLevelNormalSprites;
         textureList = &mLevelNormalTextures;
         typeStr = "normals";
         break;
      default:
         break;
   }

   printf(
      "%s texture: size: %d x %d\n",
      typeStr.c_str(),
      source.getSize().x,
      source.getSize().y
   );

   fflush(stdout);

   int width = source.getSize().x;
   int height = source.getSize().y;
   int i = 0;

   for (unsigned int x = 0; x < width; x += size)
   {
      for (unsigned int y = 0; y < height; y += size)
      {
         sf::Texture* texture = new sf::Texture();
         texture->loadFromImage(source, sf::IntRect(x, y, size, size));
         textureList->push_back(texture);

         sf::Sprite* sprite = new sf::Sprite(*texture);
         sprite->setPosition(x, y);

         spriteList->push_back(sprite);

         printf("creating %s texture %d\n", typeStr.c_str(), ++i);
         fflush(stdout);
      }
   }
}


void BulletLevel::draw(sf::RenderTarget& target) const
{
   sf::Shader* shader = mBumpMap->getShader();

   shader->setParameter("lightPos", mBumpMap->getLightPosition());

   for (int i = 0; i < mLevelColorSprites.size(); i++)
   {
      shader->setParameter(
         "normalmap",
         *mLevelNormalTextures[i]
      );

      target.draw(
         *(mLevelColorSprites[i]),
         shader
      );
   }

//   for (
//       std::vector<sf::Sprite*>::const_iterator it = mLevelColorSprites.begin();
//      it != mLevelColorSprites.end();
//      ++it
//   )
//   {
//      target.draw(*(*it));
//   }
}

