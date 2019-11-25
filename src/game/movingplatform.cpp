// base
#include "movingplatform.h"

#include "constants.h"
#include "fixturenode.h"
#include "globalclock.h"
#include "player.h"
#include "physicsconfiguration.h"
#include "tmxparser/tmximage.h"
#include "tmxparser/tmxlayer.h"
#include "tmxparser/tmxobject.h"
#include "tmxparser/tmxpolyline.h"
#include "tmxparser/tmxproperty.h"
#include "tmxparser/tmxproperties.h"
#include "tmxparser/tmxtileset.h"

#include <math.h>
#include "Box2D/Box2D.h"


//-----------------------------------------------------------------------------
MovingPlatform::MovingPlatform(GameNode *parent)
 : GameNode(parent)
{
}


//-----------------------------------------------------------------------------
void MovingPlatform::draw(sf::RenderTarget& target)
{
   for (const auto& sprite : mSprites)
   {
      target.draw(sprite);
   }
}


//-----------------------------------------------------------------------------
double MovingPlatform::CosineInterpolate(double y1, double y2, double mu)
{
   double mu2 = (1.0 - cos(mu * M_PI)) * 0.5;
   return (y1 * (1.0 - mu2) + y2 * mu2);
}


//-----------------------------------------------------------------------------
b2Body* MovingPlatform::getBody()
{
    return mBody;
}


//-----------------------------------------------------------------------------
void MovingPlatform::setOffset(float x, float y)
{
   mX = x;
   mY = y;
}


//-----------------------------------------------------------------------------
void MovingPlatform::setupTransform()
{
   auto x = mTilePosition.x * PIXELS_PER_TILE / PPM;
   auto y = mTilePosition.y * PIXELS_PER_TILE / PPM;
   mBody->SetTransform(b2Vec2(x, y), 0);
}


//-----------------------------------------------------------------------------
void MovingPlatform::setupBody(const std::shared_ptr<b2World>& world)
{
   b2PolygonShape polygonShape;
   auto sizeX = PIXELS_PER_TILE / PPM;
   auto sizeY = 0.5f * PIXELS_PER_TILE / PPM;
   b2Vec2 vertices[4];
   vertices[0] = b2Vec2(0,              0              );
   vertices[1] = b2Vec2(0,              sizeY * mHeight);
   vertices[2] = b2Vec2(sizeX * mWidth, sizeY * mHeight);
   vertices[3] = b2Vec2(sizeX * mWidth, 0              );
   polygonShape.Set(vertices, 4);

   b2BodyDef bodyDef;
   bodyDef.type = b2_kinematicBody;
   mBody = world->CreateBody(&bodyDef);
   // mBody->SetGravityScale(0.0f);

   setupTransform();

   auto fixture = mBody->CreateFixture(&polygonShape, 0);
   auto objectData = new FixtureNode(this);
   objectData->setType(ObjectTypeMovingPlatform);
   fixture->SetUserData(static_cast<void*>(objectData));
}


//-----------------------------------------------------------------------------
void MovingPlatform::addSprite(const sf::Sprite& sprite)
{
   mSprites.push_back(sprite);
}


//-----------------------------------------------------------------------------
std::vector<std::shared_ptr<GameMechanism>> MovingPlatform::load(
   TmxLayer* layer,
   TmxTileSet* tileSet,
   const std::filesystem::path& basePath,
   const std::shared_ptr<b2World>& world
)
{
   std::vector<std::shared_ptr<GameMechanism>> movingPlatforms;
   const auto tilesize = sf::Vector2u(tileSet->mTileWidth, tileSet->mTileHeight);
   const auto tiles    = layer->mData;
   const auto width    = layer->mWidth;
   const auto height   = layer->mHeight;
   const auto firstId  = tileSet->mFirstGid;

   // populate the vertex array, with one quad per tile
   for (auto y = 0u; y < height; y++)
   {
      for (auto x = 0u; x < width; x++)
      {
         // get the current tile number
         auto tileNumber = tiles[x + y * width];

         if (tileNumber != 0)
         {
            // find matching MovingPlatform
            auto movingPlatform = std::make_shared<MovingPlatform>(nullptr);
            movingPlatforms.push_back(movingPlatform);

            movingPlatform->mTilePosition.x = x;
            movingPlatform->mTilePosition.y = y;
            movingPlatform->mTexture.loadFromFile((basePath / tileSet->mImage->mSource).string());

            if (layer->mProperties != nullptr)
            {
               movingPlatform->setZ(layer->mProperties->mMap["z"]->mValueInt);
            }

            while (tileNumber != 0)
            {
               auto tileId = tileNumber - firstId;
               auto tu = (tileId) % (movingPlatform->mTexture.getSize().x / tilesize.x);
               auto tv = (tileId) / (movingPlatform->mTexture.getSize().x / tilesize.x);

               sf::Sprite sprite;
               sprite.setTexture(movingPlatform->mTexture);
               sprite.setTextureRect(
                  sf::IntRect(
                     tu * PIXELS_PER_TILE,
                     tv * PIXELS_PER_TILE,
                     PIXELS_PER_TILE,
                     PIXELS_PER_TILE
                  )
               );

               sprite.setPosition(
                  sf::Vector2f(
                     static_cast<float_t>(x * PIXELS_PER_TILE),
                     static_cast<float_t>(y * PIXELS_PER_TILE)
                  )
               );

               movingPlatform->addSprite(sprite);
               movingPlatform->mWidth++;

               // jump to next tile
               x++;
               tileNumber = tiles[x + y * width];
            }

            movingPlatform->setupBody(world);

            // printf(
            //   "created MovingPlatform %zd at %d, %d (width: %zd)\n",
            //   movingPlatforms.size(),
            //   x,
            //   y,
            //   movingPlatform->mSprites.size()
            // );
         }
      }
   }

   return movingPlatforms;
}


//-----------------------------------------------------------------------------
void MovingPlatform::link(const std::vector<std::shared_ptr<GameMechanism>>& platforms, TmxObject *tmxObject)
{
   std::vector<sf::Vector2f> polyline = tmxObject->mPolyLine->mPolyLine;

   auto pos = polyline.at(0);

   auto x = static_cast<int>((pos.x + tmxObject->mX) / PIXELS_PER_TILE);
   auto y = static_cast<int>((pos.y + tmxObject->mY) / PIXELS_PER_TILE);
   std::shared_ptr<MovingPlatform> platform;

   for (auto p : platforms)
   {
      auto tmp = std::dynamic_pointer_cast<MovingPlatform>(p);
      if (tmp->mTilePosition.y == y)
      {
         for (auto xi = 0; xi < tmp->mWidth; xi++)
         {
            if (tmp->mTilePosition.x + xi == x)
            {
               platform = tmp;
               // printf("linking tmx poly to platform at %d, %d\n", x, y);
               break;
            }
         }
      }

      if (platform != nullptr)
      {
         break;
      }
   }

   if (platform != nullptr)
   {
      auto i = 0;
      for (const auto& polyPos : polyline)
      {
         b2Vec2 platformPos;
         auto time = i / static_cast<float>(polyline.size() - 1);

         // where do those 4px error come from?!
         auto x = (tmxObject->mX + polyPos.x - 4 - (platform->mWidth * PIXELS_PER_TILE) / 2.0f) * MPP;
         auto y = (tmxObject->mY + polyPos.y - (platform->mHeight * PIXELS_PER_TILE) / 2.0f) * MPP;

         platformPos.x = x;
         platformPos.y = y;

         platform->mInterpolation.addKey(platformPos, time);
         i++;
      }
   }
}


   //  |                 |
   //  |              ____
   //  |        __----
   //  _____----         |
   //                    |
   //  +-----------------+
   //  0                 1
   //
   //  p0                pn


//-----------------------------------------------------------------------------
void MovingPlatform::update(const sf::Time& /*dt*/)
{
   // configured timestep is 1/35
   // frame update timestep is 1/60
   // causes an error
   //   pixel pos: 2808.000000, 8739.437500
   //   pixel pos: 2808.000000, 8740.535156
   // 8739.437500 - 8740.535156 = 1.097656
   // 1 / 1.097656 => 0.91103223596463737272879663574016

   const float error = 0.91192227210220912883854305376065f;

   if (mInterpolation.update(mBody->GetPosition()))
   {
      // PhysicsConfiguration::getInstance().mTimeStep
      mBody->SetLinearVelocity(error * (PPM / 60.0f) * mInterpolation.getVelocity());
   }

   auto pos = 0;
   auto horizontal = (mWidth  > 1) ? 1 : 0;
   auto vertical   = (mHeight > 1) ? 1 : 0;

   for (auto& sprite : mSprites)
   {
      auto x = mBody->GetPosition().x * PPM + horizontal * pos * PIXELS_PER_TILE;
      auto y = mBody->GetPosition().y * PPM + vertical   * pos * PIXELS_PER_TILE;

      sprite.setPosition(x, y);

      pos++;
   }
}

