// base
#include "movingplatform.h"

#include "constants.h"
#include "fixturenode.h"
#include "framework/tmxparser/tmximage.h"
#include "framework/tmxparser/tmxlayer.h"
#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxpolyline.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxtileset.h"
#include "level.h"
#include "player/player.h"
#include "physics/physicsconfiguration.h"
#include "tools/globalclock.h"

#include <iostream>
#include <math.h>

#include "Box2D/Box2D.h"


sf::Texture MovingPlatform::sTexture;


//-----------------------------------------------------------------------------
MovingPlatform::MovingPlatform(GameNode *parent)
 : GameNode(parent)
{
   setName(typeid(MovingPlatform).name());
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
const std::vector<sf::Vector2f>& MovingPlatform::getPixelPath() const
{
   return mPixelPath;
}


//-----------------------------------------------------------------------------
const PathInterpolation& MovingPlatform::getInterpolation() const
{
   return mInterpolation;
}


//-----------------------------------------------------------------------------
b2Body* MovingPlatform::getBody()
{
   return mBody;
}


//-----------------------------------------------------------------------------
void MovingPlatform::setEnabled(bool enabled)
{
   // std::cout << mInitialized << std::endl;

   GameMechanism::setEnabled(enabled);

   if (mInitialized)
   {
      mLeverLag = enabled ? 0.0f : 1.0f;
   }
   else
   {
      mInitialized = true;
   }
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
   sTexture.loadFromFile((basePath / tileSet->mImage->mSource).string());

   std::vector<std::shared_ptr<GameMechanism>> movingPlatforms;
   const auto tilesize = sf::Vector2u(tileSet->mTileWidth, tileSet->mTileHeight);
   const auto tiles    = layer->mData;
   const auto width    = layer->mWidth;
   const auto height   = layer->mHeight;
   const auto firstId  = tileSet->mFirstGid;

   for (auto y = 0u; y < height; y++)
   {
      for (auto x = 0u; x < width; x++)
      {
         // get the current tile number
         auto tileNumber = tiles[x + y * width];

         if (tileNumber != 0)
         {
            // find matching platform
            auto movingPlatform = std::make_shared<MovingPlatform>(Level::getCurrentLevel());

            // std::cout << "creating moving platform at: " << x << ", " << y << std::endl;

            movingPlatforms.push_back(movingPlatform);

            movingPlatform->mTilePosition.x = x;
            movingPlatform->mTilePosition.y = y;

            if (layer->mProperties != nullptr)
            {
               movingPlatform->setZ(layer->mProperties->mMap["z"]->mValueInt);
            }

            while (tileNumber != 0)
            {
               auto tileId = tileNumber - firstId;
               auto tu = (tileId) % (sTexture.getSize().x / tilesize.x);
               auto tv = (tileId) / (sTexture.getSize().x / tilesize.x);

               sf::Sprite sprite;
               sprite.setTexture(sTexture);
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
   std::vector<sf::Vector2f> pixelPath = tmxObject->mPolyLine->mPolyLine;

   auto pos = pixelPath.at(0);

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
      for (const auto& polyPos : pixelPath)
      {
         b2Vec2 platformPos;
         auto time = i / static_cast<float>(pixelPath.size() - 1);

         // where do those 4px error come from?!
         auto x = (tmxObject->mX + polyPos.x - 4 - (platform->mWidth  * PIXELS_PER_TILE) / 2.0f) * MPP;
         auto y = (tmxObject->mY + polyPos.y -     (platform->mHeight * PIXELS_PER_TILE) / 2.0f) * MPP;

         platformPos.x = x;
         platformPos.y = y;

         platform->mInterpolation.addKey(platformPos, time);
         platform->mPixelPath.push_back({(pos.x + tmxObject->mX), (pos.y + tmxObject->mY)});

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
void MovingPlatform::updateLeverLag(const sf::Time& dt)
{
   if (!isEnabled())
   {
      if (mLeverLag <= 0.0f)
      {
         mLeverLag = 0.0f;
      }
      else
      {
         mLeverLag -= dt.asSeconds();
      }
   }
   else
   {
      if (mLeverLag < 1.0f)
      {
         mLeverLag += dt.asSeconds();
      }
      else
      {
         mLeverLag = 1.0f;
      }
   }
}


//-----------------------------------------------------------------------------
void MovingPlatform::update(const sf::Time& dt)
{
   updateLeverLag(dt);

   mInterpolation.update(mBody->GetPosition());

   mBody->SetLinearVelocity(mLeverLag * TIMESTEP_ERROR * (PPM / 60.0f) * mInterpolation.getVelocity());

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

