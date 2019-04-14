#include "door.h"

// game
#include "constants.h"
#include "fixturenode.h"
#include "player.h"
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
void Door::update(float dt)
{
   auto playerPos = Player::getPlayer(0)->getPixelPosition();
   auto doorPos = mSprites.at(mSprites.size()- 1 ).getPosition();

   sf::Vector2f a(playerPos.x, playerPos.y);
   sf::Vector2f b(doorPos.x + TILE_WIDTH * 0.5f, doorPos.y);

   auto distance = SfmlMath::length(a - b);
   auto atDoor = (distance < TILE_WIDTH * 1.5f);

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
            static_cast<float>(x * TILE_WIDTH),
            static_cast<float>((i + y) * TILE_HEIGHT  + mOffset)
         )
      );

      i++;
   }

   auto openSpeed = 50.0f;

   switch (mState)
   {
      case State::Opening:
      {
         mOffset -= openSpeed * dt;
         if (fabs(mOffset) >= TILE_HEIGHT * mHeight)
         {
            mState = State::Open;
            mOffset = static_cast<float_t>(-TILE_HEIGHT * mHeight);
         }
         break;
      }
      case State::Closing:
      {
         mOffset += openSpeed * dt;
         if (mOffset >= 0.0f)
         {
            mState = State::Closed;
            mOffset = 0.0f;
         }
         break;
      }
      case State::Open:
      case State::Closed:
      default:
         break;
   }

   updateTransform();
}


//-----------------------------------------------------------------------------
void Door::updateTransform()
{
   auto x =            mTilePosition.x * TILE_WIDTH / PPM;
   auto y = (mOffset + mTilePosition.y * TILE_HEIGHT) / PPM;
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
      default:
         break;
   }
}


//-----------------------------------------------------------------------------
void Door::setupBody(const std::shared_ptr<b2World>& world)
{
   b2PolygonShape polygonShape;
   auto sizeX = (TILE_WIDTH / PPM) * 0.26f;
   auto sizeY = (TILE_HEIGHT / PPM);
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
   fixture->SetUserData((void*)objectData);
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
   // TODO
   // add door locking mechanism
   //   if (mTileId == 161)
   //   {
   //      return;
   //   }

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
      default:
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
int Door::getZ() const
{
   return mZ;
}


//-----------------------------------------------------------------------------
void Door::setZ(int z)
{
   mZ = z;
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
std::vector<Door *> Door::load(
   TmxLayer* layer,
   TmxTileSet* tileSet,
   const std::filesystem::path& basePath,
   const std::shared_ptr<b2World>& world
)
{
   std::vector<Door*> doors;

   auto tilesize = sf::Vector2u(tileSet->mTileWidth, tileSet->mTileHeight);
   auto tiles    = layer->mData;
   auto width    = layer->mWidth;
   auto height   = layer->mHeight;
   auto firstId  = tileSet->mFirstGid;

   // populate the vertex array, with one quad per tile
   for (auto j = 0; j < height; j++)
   {
      for (auto i = 0; i < width; i++)
      {
         // get the current tile number
         auto tileNumber = tiles[i + j * width];

         if (tileNumber != 0)
         {
            auto tileId = tileNumber - firstId;

            // find matching door
            Door* door = nullptr;
            for (Door* tmp : doors)
            {
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
               door = new Door(nullptr);
               doors.push_back(door);

               door->mTileId = tileId;
               door->mTilePosition.x = i;
               door->mTilePosition.y = j;
               door->mTexture.loadFromFile((basePath / tileSet->mImage->mSource).string());

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
                  tu * TILE_WIDTH,
                  tv * TILE_HEIGHT,
                  TILE_WIDTH,
                  TILE_HEIGHT
               )
            );

            sprite.setPosition(
               sf::Vector2f(
                  static_cast<float_t>(i * TILE_WIDTH),
                  static_cast<float_t>(j * TILE_HEIGHT)
               )
            );

            door->addSprite(sprite);
         }
      }
   }

   for (Door* tmp : doors)
   {
      tmp->setupBody(world);
   }

   return doors;
}


