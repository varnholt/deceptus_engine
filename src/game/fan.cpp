#include "fan.h"

#include "constants.h"
#include "tmxparser/tmxlayer.h"
#include "tmxparser/tmxobject.h"
#include "tmxparser/tmxtileset.h"

#include <iostream>


std::vector<Fan*> Fan::sFans;
std::vector<Fan::FanTile*> Fan::sTiles;
std::vector<TmxObject*> Fan::sObjects;
std::vector<sf::Vector2f> Fan::sWeights;


static const sf::Vector2f vectorUp{0.0f, 1.0f};
static const sf::Vector2f vectorLeft{-1.0f, 0.0f};
static const sf::Vector2f vectorRight{1.0f, 0.0f};


void Fan::createPhysics(const std::shared_ptr<b2World>& world, FanTile* tile)
{
   auto possf = tile->mPosition;
   auto posb2d = b2Vec2(possf.x * MPP, possf.y * MPP);

   b2BodyDef bodyDef;
   bodyDef.type = b2_staticBody;
   bodyDef.position = posb2d;
   tile->mBody = world->CreateBody(&bodyDef);

   auto width = PIXELS_PER_TILE * MPP * 0.5f;
   auto height = PIXELS_PER_TILE * MPP * 0.5f;

   // create fixture for physical boundaries of the belt object
   b2PolygonShape shape;
   shape.SetAsBox(
     width, height,
     b2Vec2(width, height),
     0.0f
   );

   b2FixtureDef boundaryFixtureDef;
   boundaryFixtureDef.shape = &shape;
   boundaryFixtureDef.density = 1.0f;
   boundaryFixtureDef.isSensor = false;
   tile->mBody->CreateFixture(&boundaryFixtureDef);
}


void Fan::load(
   TmxLayer* layer,
   TmxTileSet* tileSet,
   const std::shared_ptr<b2World>& world
)
{
   const auto tiles    = layer->mData;
   const auto width    = layer->mWidth;
   const auto height   = layer->mHeight;
   const auto firstId  = tileSet->mFirstGid;

   // populate the vertex array, with one quad per tile
   for (auto i = 0u; i < width; ++i)
   {
      for (auto j = 0u; j < height; ++j)
      {
         // get the current tile number
         int tileNumber = tiles[i + j * width];

         if (tileNumber != 0)
         {
            // std::cout << tileNumber - firstId << std::endl;

            const auto direction = static_cast<DirectionTile>(tileNumber - firstId);
            sf::Vector2f directionVector;

            switch (direction)
            {
               case DirectionTile::Up:
                  directionVector = vectorUp;
                  break;
               case DirectionTile::Left:
                  directionVector = vectorLeft;
                  break;
               case DirectionTile::Right:
                  directionVector = vectorRight;
                  break;
            }

            FanTile* tile = new FanTile();
            tile->mPosition = sf::Vector2i(i * PIXELS_PER_TILE, j * PIXELS_PER_TILE);
            tile->mRect.left = i * PIXELS_PER_TILE;
            tile->mRect.top = j * PIXELS_PER_TILE;
            tile->mRect.width = PIXELS_PER_TILE;
            tile->mRect.height = PIXELS_PER_TILE;
            tile->mDirection = directionVector;
            sTiles.push_back(tile);

            createPhysics(world, tile);
         }
      }
   }
}


void Fan::addObject(TmxObject* object)
{
   sObjects.push_back(object);

   Fan* fan = new Fan();
   sFans.push_back(fan);

   const auto w = static_cast<int32_t>(object->mWidth);
   const auto h = static_cast<int32_t>(object->mHeight);

   fan->mRect.left = static_cast<int32_t>(object->mX);
   fan->mRect.top = static_cast<int32_t>(object->mY);
   fan->mRect.width = w;
   fan->mRect.height = h;
}


std::optional<sf::Vector2f> Fan::collide(const sf::Rect<int32_t>& playerRect)
{
   // need to find all intersections since there can be more than one
   auto valid = false;
   sf::Vector2f dir;

   for (auto fan : sFans)
   {
      if (playerRect.intersects(fan->mRect))
      {
         dir += fan->mDirection;
         valid = true;
      }
   }

   if (valid)
   {
      // std::cout << "dir: " << dir.x << ", " << dir.y << std::endl;
      return dir;
   }
   else
   {
      return {};
   }
}


void Fan::merge()
{
   for (auto tile : sTiles)
   {
      for (auto fan : sFans)
      {
         if (tile->mRect.intersects(fan->mRect))
         {
            fan->mTiles.push_back(tile);
            fan->mDirection = tile->mDirection;
         }
      }
   }

   sTiles.clear();
}


Fan::FanTile::~FanTile()
{
   mBody->GetWorld()->DestroyBody(mBody);
}


