#include "door.h"

// game
#include "constants.h"
#include "fixturenode.h"
#include "level.h"
#include "math/sfmlmath.h"
#include "player/player.h"
#include "savestate.h"
#include "tools/timer.h"

#include "framework/tmxparser/tmximage.h"
#include "framework/tmxparser/tmxlayer.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxtileset.h"

#include <iostream>


sf::Texture Door::sTexture;


//-----------------------------------------------------------------------------
Door::Door(GameNode* parent)
 : GameNode(parent)
{
   setName(typeid(Door).name());
}


//-----------------------------------------------------------------------------
void Door::draw(sf::RenderTarget& window)
{
   window.draw(mSpriteIcon);
   window.draw(mDoorQuad, &sTexture);
}


//-----------------------------------------------------------------------------
void Door::updateBars(const sf::Time& dt)
{
   const float left   = 0.0f;
   const float right  = left + 3.0f * PIXELS_PER_TILE;
   const float top    = 3.0f * PIXELS_PER_TILE - mOffset;
   const float bottom = top + 3.0f * PIXELS_PER_TILE;

   mDoorQuad[0].texCoords = sf::Vector2f(left, top);
   mDoorQuad[1].texCoords = sf::Vector2f(left, bottom);
   mDoorQuad[2].texCoords = sf::Vector2f(right, bottom);
   mDoorQuad[3].texCoords = sf::Vector2f(right, top);

   auto openSpeed = 50.0f;
   auto closeSpeed = 200.0f;

   switch (mState)
   {
      case State::Opening:
      {
         mOffset -= openSpeed * dt.asSeconds();
         if (fabs(mOffset) >= PIXELS_PER_TILE * mHeight)
         {
            mState = State::Open;
            mOffset = static_cast<float_t>(-PIXELS_PER_TILE * mHeight);
         }
         break;
      }
      case State::Closing:
      {
         mOffset += closeSpeed * dt.asSeconds();
         if (mOffset >= 0.0f)
         {
            mState = State::Closed;
            mOffset = 0.0f;
         }
         break;
      }
      case State::Open:
      case State::Closed:
         break;
   }

   updateTransform();
}


//-----------------------------------------------------------------------------
void Door::update(const sf::Time& dt)
{
   switch (mType)
   {
      case Type::Conventional:
      {
         break;
      }
      case Type::Bars:
      {
         updateBars(dt);
         // updateBars(dt);
         break;
      }
   }
}


//-----------------------------------------------------------------------------
void Door::updateTransform()
{
   auto x =            mTilePosition.x * PIXELS_PER_TILE / PPM;
   auto y = (mOffset + mTilePosition.y * PIXELS_PER_TILE) / PPM;
   mBody->SetTransform(b2Vec2(x, y), 0);
}


//-----------------------------------------------------------------------------
void Door::reset()
{
   mState = mInitialState;

   switch (mState)
   {
      case State::Open:
         mOffset = 1.0f;
         break;
      case State::Closed:
         mOffset = 0.0f;
         break;
      case State::Opening:
      case State::Closing:
         break;
   }
}


//-----------------------------------------------------------------------------
void Door::setupBody(
   const std::shared_ptr<b2World>& world,
   float xOffset,
   float xScale
)
{
   b2PolygonShape polygonShape;
   auto sizeX = (PIXELS_PER_TILE / PPM) * xScale;
   auto sizeY = (PIXELS_PER_TILE / PPM);

   b2Vec2 vertices[4];
   vertices[0] = b2Vec2(xOffset,         0);
   vertices[1] = b2Vec2(xOffset,         mHeight * sizeY);
   vertices[2] = b2Vec2(xOffset + sizeX, mHeight * sizeY);
   vertices[3] = b2Vec2(xOffset + sizeX, 0);
   polygonShape.Set(vertices, 4);

   b2BodyDef bodyDef;
   bodyDef.type = b2_kinematicBody;
   mBody = world->CreateBody(&bodyDef);

   updateTransform();

   auto fixture = mBody->CreateFixture(&polygonShape, 0);
   auto objectData = new FixtureNode(this);
   objectData->setType(ObjectTypeDoor);
   fixture->SetUserData(static_cast<void*>(objectData));
}


//-----------------------------------------------------------------------------
bool Door::checkPlayerAtDoor() const
{
   auto playerPos = Player::getCurrent()->getPixelPositionf();
   auto doorPos = mSpriteIcon.getPosition();

   sf::Vector2f a(playerPos.x, playerPos.y);
   sf::Vector2f b(doorPos.x + PIXELS_PER_TILE * 0.5f, doorPos.y + 3 * PIXELS_PER_TILE);

   auto distance = SfmlMath::length(a - b);
   auto atDoor = (distance < PIXELS_PER_TILE * 1.5f);

   // std::cout << fabs(a.x - b.x) << std::endl;
   // std::cout << fabs(a.y - b.y) << std::endl;

   return atDoor;
}


//-----------------------------------------------------------------------------
void Door::toggle()
{
   if (!SaveState::getPlayerInfo().mInventory.hasInventoryItem(mRequiredItem))
   {
      // std::cout << "player doesn't have key" << std::endl;
      return;
   }

   if (!checkPlayerAtDoor())
   {
      // std::cout << "player not in front of door" << std::endl;
      return;
   }

   switch (mState)
   {
      case State::Open:
         close();
         break;
      case State::Closed:
         open();
         break;
      case State::Opening:
      case State::Closing:
         break;
   }
}


//-----------------------------------------------------------------------------
void Door::open()
{
   mState = State::Opening;
   Timer::add(std::chrono::milliseconds(10000), [this](){close();}, Timer::Type::Singleshot);
}


//-----------------------------------------------------------------------------
void Door::close()
{
   mState = State::Closing;
}


//-----------------------------------------------------------------------------
const sf::Vector2i& Door::getTilePosition() const
{
   return mTilePosition;
}


//-----------------------------------------------------------------------------
bool Door::isPlayerAtDoor() const
{
   return mPlayerAtDoor;
}



//-----------------------------------------------------------------------------
void Door::setPlayerAtDoor(bool playerAtDoor)
{
   mPlayerAtDoor = playerAtDoor;
}


//-----------------------------------------------------------------------------
std::vector<std::shared_ptr<GameMechanism>> Door::load(
   TmxLayer* layer,
   TmxTileSet* tileSet,
   const std::filesystem::path& basePath,
   const std::shared_ptr<b2World>& world
)
{
   sTexture.loadFromFile((basePath / tileSet->mImage->mSource).string());

   std::vector<std::shared_ptr<GameMechanism>> doors;

   auto tiles    = layer->mData;
   auto width    = layer->mWidth;
   auto height   = layer->mHeight;
   auto firstId  = tileSet->mFirstGid;

   // populate the vertex array, with one quad per tile
   for (auto j = 0u; j < height; j++)
   {
      for (auto i = 0u; i < width; i++)
      {
         // get the current tile number
         auto tileNumber = tiles[i + j * width];

         if (tileNumber != 0)
         {
            auto tileId = tileNumber - firstId;

            // 21: red
            // 24: green
            // 27: blue
            // ...

            auto requiredItem = ItemType::Invalid;
            auto iconOffset = 0;
            auto createDoor = false;

            switch (tileId)
            {
               case 21:
                  requiredItem = ItemType::KeyRed;
                  iconOffset = 1;
                  createDoor = true;
                  break;
               case 24:
                  requiredItem = ItemType::KeyGreen;
                  iconOffset = 4;
                  createDoor = true;
                  break;
               case 27:
                  requiredItem = ItemType::KeyBlue;
                  iconOffset = 7;
                  createDoor = true;
                  break;
               case 30:
                  requiredItem = ItemType::KeyYellow;
                  iconOffset = 10;
                  createDoor = true;
                  break;
               case 33:
                  requiredItem = ItemType::KeyOrange;
                  iconOffset = 13;
                  createDoor = true;
                  break;
               case 36:
                  requiredItem = ItemType::Invalid;
                  createDoor = true;
                  break;

               default:
                  break;
            }

            if (createDoor)
            {
               const auto positionX = static_cast<float>(i * PIXELS_PER_TILE - PIXELS_PER_TILE);
               const auto positionY = static_cast<float>(j * PIXELS_PER_TILE + PIXELS_PER_TILE);

               auto door = std::make_shared<Door>(Level::getCurrentLevel());
               doors.push_back(door);

               door->mDoorQuad[0].position.x = positionX;
               door->mDoorQuad[0].position.y = positionY;
               door->mDoorQuad[1].position.x = positionX;
               door->mDoorQuad[1].position.y = positionY + 3 * PIXELS_PER_TILE;
               door->mDoorQuad[2].position.x = positionX + 3 * PIXELS_PER_TILE;
               door->mDoorQuad[2].position.y = positionY + 3 * PIXELS_PER_TILE;
               door->mDoorQuad[3].position.x = positionX + 3 * PIXELS_PER_TILE;
               door->mDoorQuad[3].position.y = positionY;
               door->mType = Type::Bars;
               door->mTileId = tileId;
               door->mTilePosition.x = static_cast<int32_t>(i);
               door->mTilePosition.y = static_cast<int32_t>(j) + 1; // the actual door is a tile lower
               door->mRequiredItem = requiredItem;
               door->mHeight = 3; // hardcoded 3 tiles

               if (requiredItem != ItemType::Invalid)
               {
                  door->mSpriteIcon.setTexture(sTexture);
                  door->mSpriteIcon.setTextureRect(sf::IntRect(PIXELS_PER_TILE * iconOffset, PIXELS_PER_TILE, PIXELS_PER_TILE, PIXELS_PER_TILE));
                  door->mSpriteIcon.setPosition(
                     static_cast<float>(i * PIXELS_PER_TILE),
                     static_cast<float>(j * PIXELS_PER_TILE)
                  );
               }

               if (layer->mProperties != nullptr)
               {
                  door->setZ(layer->mProperties->mMap["z"]->mValueInt);
               }
            }

            // std::cout << "found new door: " << tileId << std::endl;
         }
      }
   }

   for (auto tmp : doors)
   {
      std::dynamic_pointer_cast<Door>(tmp)->setupBody(world);
   }

   return doors;
}
