#include "deathblock.h"

#include "tmxparser/tmxobject.h"
#include "tmxparser/tmxpolyline.h"

#include "constants.h"
#include "fixturenode.h"
#include "player.h"
#include "texturepool.h"

#include <iostream>


sf::Texture DeathBlock::sTexture;


DeathBlock::DeathBlock(GameNode* parent)
 : GameNode(parent)
{
   setName("DeathBlock");
}


void DeathBlock::draw(sf::RenderTarget& target)
{
   for (auto& sprite : mSprites)
   {
      target.draw(sprite);
   }
}


// enemy_deathblock
// 14 animation cycles
// 0: spikes out
// 13: spikes in
//
// sprite setup:
//
//           +---+
//           | 0 |
//       +---+---+---+
//       | 1 | 2 | 3 |
//       +---+---+---+
//           | 4 |
//           +---+
//
// offsets:
//
//    0: 1, 0
//    1: 0, 1
//    2: 1, 1
//    3: 2, 1
//    4: 1, 2


//-----------------------------------------------------------------------------
void DeathBlock::setupTransform()
{
   auto x = mPixelPosition.x / PPM - (PIXELS_PER_TILE / (2 * PPM));
   auto y = mPixelPosition.y / PPM;
   mBody->SetTransform(b2Vec2(x, y), 0);
}


//-----------------------------------------------------------------------------
void DeathBlock::setupBody(const std::shared_ptr<b2World>& world)
{
   b2PolygonShape polygonShape;

   auto sizeX = PIXELS_PER_TILE / PPM;
   auto sizeY = PIXELS_PER_TILE / PPM;

   b2Vec2 vertices[4];
   vertices[0] = b2Vec2(0,     0);
   vertices[1] = b2Vec2(0,     sizeY);
   vertices[2] = b2Vec2(sizeX, sizeY);
   vertices[3] = b2Vec2(sizeX, 0);

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


void DeathBlock::updateLeverLag(const sf::Time& dt)
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

void DeathBlock::updateCollision()
{
    // check for intersection with player
    auto playerRect = Player::getCurrent()->getPlayerPixelRect();

    auto x = mBody->GetPosition().x * PPM - PIXELS_PER_TILE;
    auto y = mBody->GetPosition().y * PPM - PIXELS_PER_TILE;

    // want a copy of the original rect
    for (auto rect : mCollisionRects)
    {
        rect.left += x;
        rect.top += y;

        if (playerRect.intersects(rect))
        {
           Player::getCurrent()->damage(100);
        }
    }
}


void DeathBlock::update(const sf::Time& dt)
{
   updateLeverLag(dt);

   mInterpolation.update(mBody->GetPosition());
   {
      mBody->SetLinearVelocity(mLeverLag * TIMESTEP_ERROR * (PPM / 60.0f) * mInterpolation.getVelocity());
   }

   for (auto i = 0u; i < mSprites.size(); i++)
   {
      mSprites[i].setTextureRect(
         sf::IntRect(
            mOffsets[i].x * PIXELS_PER_TILE + mStates[i] * PIXELS_PER_TILE,
            mOffsets[i].y * PIXELS_PER_TILE + mStates[i] * PIXELS_PER_TILE,
            PIXELS_PER_TILE,
            PIXELS_PER_TILE
         )
      );

      // need to move by one tile because the center is not 0, 0 but -24, -24
      auto x = mBody->GetPosition().x * PPM + mOffsets[i].x * PIXELS_PER_TILE - PIXELS_PER_TILE;
      auto y = mBody->GetPosition().y * PPM + mOffsets[i].y * PIXELS_PER_TILE - PIXELS_PER_TILE;

      mSprites[i].setPosition(x, y);
   }

   updateCollision();
}


void DeathBlock::setup(
   TmxObject* tmxObject,
   const std::shared_ptr<b2World>& world
)
{
   if (sTexture.getSize().x == 0)
   {
      sTexture = *TexturePool::getInstance().get("data/sprites/enemy_deathblock.png");
   }

   for (auto& sprite : mSprites)
   {
      sprite.setTexture(sTexture);
   }

   setZ(ZDepthForegroundMin + 1);

   mPixelPosition.x = tmxObject->mX;
   mPixelPosition.y = tmxObject->mY;

   setupBody(world);

   std::vector<sf::Vector2f> pixelPath = tmxObject->mPolyLine->mPolyLine;
   auto pos = pixelPath.at(0);

   auto i = 0;
   for (const auto& polyPos : pixelPath)
   {
      b2Vec2 worldPos;
      auto time = i / static_cast<float>(pixelPath.size() - 1);

      auto x = (tmxObject->mX + polyPos.x - (PIXELS_PER_TILE) / 2.0f) * MPP;
      auto y = (tmxObject->mY + polyPos.y - (PIXELS_PER_TILE) / 2.0f) * MPP;

      worldPos.x = x;
      worldPos.y = y;

      mInterpolation.addKey(worldPos, time);
      mPixelPath.push_back({(pos.x + tmxObject->mX), (pos.y + tmxObject->mY)});

      // std::cout << "world: " << x << ", " << y << " pixel: " << tmxObject->mX << ", " << tmxObject->mY << std::endl;

      i++;
   }
}
