#include "door.h"

// game
#include "constants.h"
#include "fixturenode.h"
#include "player.h"
#include "savestate.h"
#include "sfmlmath.h"
#include "timer.h"

#include "tmxparser/tmximage.h"
#include "tmxparser/tmxlayer.h"
#include "tmxparser/tmxproperty.h"
#include "tmxparser/tmxproperties.h"
#include "tmxparser/tmxtileset.h"



//-----------------------------------------------------------------------------
Door::Door(GameNode* parent)
 : GameNode(parent)
{
}


//-----------------------------------------------------------------------------
void Door::draw(sf::RenderTarget& window)
{
   for (const auto& sprite : mSprites)
   {
      window.draw(sprite);
   }
}


//-----------------------------------------------------------------------------
void Door::updateBars(const sf::Time& dt)
{
   auto openSpeed = 50.0f;

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
         mOffset += openSpeed * dt.asSeconds();
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
void Door::updateConventional(const sf::Time& /*dt*/)
{

}


//-----------------------------------------------------------------------------
void Door::update(const sf::Time& dt)
{
   auto playerPos = Player::getCurrent()->getPixelPosition();
   auto doorPos = mSprites.at(mSprites.size()- 1 ).getPosition();

   sf::Vector2f a(playerPos.x, playerPos.y);
   sf::Vector2f b(doorPos.x + PIXELS_PER_TILE * 0.5f, doorPos.y);

   auto distance = SfmlMath::length(a - b);
   auto atDoor = (distance < PIXELS_PER_TILE * 1.5f);

   setPlayerAtDoor(atDoor);

   auto i = 0;
   for (auto& sprite : mSprites)
   {
      sprite.setColor(
         sf::Color(
            255,
            atDoor ? 150 : 255,
            atDoor ? 150 : 255
         )
      );

      auto x = mTilePosition.x;
      auto y = mTilePosition.y;

      sprite.setPosition(
         sf::Vector2f(
            static_cast<float>(x * PIXELS_PER_TILE),
            static_cast<float>((i + y) * PIXELS_PER_TILE  + mOffset)
         )
      );

      i++;
   }

   switch (mType)
   {
      case Type::Conventional:
      {
         break;
      }
      case Type::Bars:
      {
         updateBars(dt);
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
void Door::setupBody(const std::shared_ptr<b2World>& world)
{
   b2PolygonShape polygonShape;
   auto sizeX = (PIXELS_PER_TILE / PPM) * 0.26f;
   auto sizeY = (PIXELS_PER_TILE / PPM);
   auto offsetX = 0.17f;
   b2Vec2 vertices[4];
   vertices[0] = b2Vec2(offsetX,         0);
   vertices[1] = b2Vec2(offsetX,         mHeight * sizeY);
   vertices[2] = b2Vec2(offsetX + sizeX, mHeight * sizeY);
   vertices[3] = b2Vec2(offsetX + sizeX, 0);
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
/*!
 * \brief Door::addSprite
 * \param sprite
 */
void Door::addSprite(const sf::Sprite & sprite)
{
   mSprites.push_back(sprite);
}


//-----------------------------------------------------------------------------
void Door::toggle()
{
   if (!SaveState::getPlayerInfo().mInventory.hasInventoryItem(mRequiredItem))
   {
      return;
   }

   if (!isPlayerAtDoor())
      return;

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
void Door::updateRequiredItem()
{
   // that code depends on the current sprite set, needs to be updated later

   auto requiredItem = ItemType::Invalid;
   switch (mTileId)
   {
      case 160:
      case 176:
      case 192:
         requiredItem = ItemType::KeyRed;
         break;
      case 161:
      case 177:
      case 193:
         requiredItem = ItemType::KeyYellow;
         break;
      case 162:
      case 178:
      case 194:
         requiredItem = ItemType::KeyBlue;
         break;
      case 163:
      case 179:
      case 195:
         requiredItem = ItemType::KeyGreen;
         break;
   }

   mRequiredItem = requiredItem;
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
   std::vector<std::shared_ptr<GameMechanism>> doors;

   auto tilesize = sf::Vector2u(static_cast<uint32_t>(tileSet->mTileWidth), static_cast<uint32_t>(tileSet->mTileHeight));
   auto tiles    = layer->mData;
   auto width    = layer->mWidth;
   auto height   = layer->mHeight;
   auto firstId  = tileSet->mFirstGid;

   // populate the vertex array, with one quad per tile
   for (auto j = 0; j < static_cast<int32_t>(height); j++)
   {
      for (auto i = 0; i < static_cast<int32_t>(width); i++)
      {
         // get the current tile number
         auto tileNumber = tiles[i + j * width];

         if (tileNumber != 0)
         {
            auto tileId = tileNumber - firstId;

            // find matching door
            std::shared_ptr<Door> door;
            for (auto& d : doors)
            {
               auto tmp = std::dynamic_pointer_cast<Door>(d);
               if (
                     (tmp->mTilePosition.x == i && tmp->mTilePosition.y + 1 == j)
                  || (tmp->mTilePosition.x == i && tmp->mTilePosition.y + 2 == j)
               )
               {
                  door = tmp;
                  break;
               }
            }

            if (door == nullptr)
            {
               door = std::make_shared<Door>(nullptr);
               doors.push_back(door);

               door->mTileId = tileId;
               door->mTilePosition.x = i;
               door->mTilePosition.y = j;
               door->mTexture.loadFromFile((basePath / tileSet->mImage->mSource).string());

               // check if door needs a key
               door->updateRequiredItem();

               // printf("creating door %zd with tile %d at %d, %d\n", doors.size(), tileId, i, j);

               if (layer->mProperties != nullptr)
               {
                  door->setZ(layer->mProperties->mMap["z"]->mValueInt);
               }
            }

            // update door height with every new tile
            door->mHeight++;

            auto tu = (tileId) % (door->mTexture.getSize().x / tilesize.x);
            auto tv = (tileId) / (door->mTexture.getSize().x / tilesize.x);

            sf::Sprite sprite;
            sprite.setTexture(door->mTexture);
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
                  static_cast<float_t>(i * PIXELS_PER_TILE),
                  static_cast<float_t>(j * PIXELS_PER_TILE)
               )
            );

            door->addSprite(sprite);
         }
      }
   }

   for (auto tmp : doors)
   {
      std::dynamic_pointer_cast<Door>(tmp)->setupBody(world);
   }

   return doors;
}


