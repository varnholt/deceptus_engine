#include "deathblock.h"

#include "tmxparser/tmxobject.h"
#include "tmxparser/tmxpolyline.h"

#include "constants.h"
#include "fixturenode.h"

#include <iostream>


DeathBlock::DeathBlock(GameNode* parent)
 : GameNode(parent)
{

}


void DeathBlock::draw(sf::RenderTarget& /*window*/)
{

}


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

   // configured timestep is 1/35
   // frame update timestep is 1/60
   // causes an error
   //   pixel pos: 2808.000000, 8739.437500
   //   pixel pos: 2808.000000, 8740.535156
   // 8739.437500 - 8740.535156 = 1.097656
   // 1 / 1.097656 => 0.91103223596463737272879663574016

   const float error = 0.91192227210220912883854305376065f;

   // if (mInterpolation.update(mBody->GetPosition()))
   mInterpolation.update(mBody->GetPosition());
   {
      // PhysicsConfiguration::getInstance().mTimeStep
      mBody->SetLinearVelocity(mLeverLag * error * (PPM / 60.0f) * mInterpolation.getVelocity());
   }

   for (auto& sprite : mSprites)
   {
      auto x = mBody->GetPosition().x * PPM + PIXELS_PER_TILE;
      auto y = mBody->GetPosition().y * PPM + PIXELS_PER_TILE;

      sprite.setPosition(x, y);
   }
}


void DeathBlock::setup(TmxObject *tmxObject, const std::shared_ptr<b2World>& world)
{
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
