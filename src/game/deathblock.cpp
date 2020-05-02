#include "deathblock.h"

#include "tmxparser/tmxobject.h"
#include "tmxparser/tmxpolyline.h"

#include "constants.h"
#include "fixturenode.h"
#include "texturepool.h"

#include <iostream>


sf::Texture DeathBlock::sTexture;


DeathBlock::DeathBlock(GameNode* parent)
 : GameNode(parent)
{
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


enum SpikeOrientation
{
   Up     = 0,
   Left   = 1,
   Center = 2,
   Right  = 3,
   Down   = 4
};


//-----------------------------------------------------------------------------
void DeathBlock::setupTransform()
{
   auto x = mTilePosition.x * PIXELS_PER_TILE / PPM;
   auto y = mTilePosition.y * PIXELS_PER_TILE / PPM;
   mBody->SetTransform(b2Vec2(x, y), 0);
}


//-----------------------------------------------------------------------------
void DeathBlock::setupBody(const std::shared_ptr<b2World>& world)
{
   b2PolygonShape polygonShape;

   auto sizeX = PIXELS_PER_TILE / PPM;
   auto sizeY = 0.5f * PIXELS_PER_TILE / PPM;

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


void DeathBlock::update(const sf::Time& dt)
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

   mInterpolation.update(mBody->GetPosition());
   {
      mBody->SetLinearVelocity(mLeverLag * TIMESTEP_ERROR * (PPM / 60.0f) * mInterpolation.getVelocity());
   }

   for (auto i = 0u; i < mSprites.size(); i++)
   {
      mSprites[i].setTextureRect(
         sf::IntRect(
            mOffsets[i].x * PIXELS_PER_TILE + mStates[i] * PIXELS_PER_TILE,
            mOffsets[i].x * PIXELS_PER_TILE + mStates[i] * PIXELS_PER_TILE,
            PIXELS_PER_TILE,
            PIXELS_PER_TILE
         )
      );
   }

   for (auto& sprite : mSprites)
   {
      auto x = mBody->GetPosition().x * PPM + PIXELS_PER_TILE;
      auto y = mBody->GetPosition().y * PPM + PIXELS_PER_TILE;

      sprite.setPosition(x, y);
   }
}


void DeathBlock::setup(
   TmxObject *tmxObject,
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

   std::shared_ptr<DeathBlock> deathBlock = std::make_shared<DeathBlock>(nullptr);

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

      deathBlock->mInterpolation.addKey(worldPos, time);
      deathBlock->mPixelPath.push_back({(pos.x + tmxObject->mX), (pos.y + tmxObject->mY)});

      std::cout << "world: " << x << ", " << y << " pixel: " << tmxObject->mX << ", " << tmxObject->mY << std::endl;

      i++;
   }
}
