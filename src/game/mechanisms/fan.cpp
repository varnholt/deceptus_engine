#include "fan.h"

#include "constants.h"
#include "framework/tmxparser/tmxlayer.h"
#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxtileset.h"

#include "texturepool.h"

#include <iostream>


std::vector<std::shared_ptr<GameMechanism>> Fan::sFans;
std::vector<std::shared_ptr<Fan::FanTile>> Fan::sTiles;
std::vector<TmxObject*> Fan::sObjects;
std::vector<sf::Vector2f> Fan::sWeights;


static const sf::Vector2f vectorUp{0.0f, 1.0f};
static const sf::Vector2f vectorDown{0.0f, -1.0f};
static const sf::Vector2f vectorLeft{-1.0f, 0.0f};
static const sf::Vector2f vectorRight{1.0f, 0.0f};


void Fan::createPhysics(const std::shared_ptr<b2World>& world, const std::shared_ptr<FanTile>& tile)
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


const sf::Rect<int32_t>& Fan::getPixelRect() const
{
   return mPixelRect;
}


void Fan::setEnabled(bool enabled)
{
   GameMechanism::setEnabled(enabled);
   mLeverLag = enabled ? 0.0f : 1.0f;
}


std::vector<std::shared_ptr<GameMechanism> >& Fan::getFans()
{
   return sFans;
}


void Fan::updateSprite()
{
   auto yOffset = 0;
   const auto dir = mTiles.at(0)->mDir;
   switch (dir)
   {
      case TileDirection::Up:
         yOffset = 0;
         break;
      case TileDirection::Right:
         yOffset = 1;
         break;
      case TileDirection::Left:
         yOffset = 2;
         break;
      case TileDirection::Down:
         yOffset = 3;
         break;
   }

   auto index = 0u;
   for (auto& sprite : mSprites)
   {
      auto xOffset = static_cast<int32_t>(mXOffsets[index]) % 8;
      sprite.setTextureRect({
         xOffset * PIXELS_PER_TILE,
         yOffset * PIXELS_PER_TILE,
         PIXELS_PER_TILE,
         PIXELS_PER_TILE
      });

      index++;
   }
}


void Fan::draw(sf::RenderTarget& target)
{
   for (auto& sprite : mSprites)
   {
      target.draw(sprite);
   }
}


void Fan::update(const sf::Time& dt)
{
   if (!isEnabled())
   {
      if (mLeverLag <= 0.0f)
      {
         return;
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

   for (auto& xOffset : mXOffsets)
   {
      xOffset += dt.asSeconds() * 25.0f * mSpeed * mLeverLag;
   }

   updateSprite();
}


void Fan::load(
   TmxLayer* layer,
   TmxTileSet* tileSet,
   const std::shared_ptr<b2World>& world
)
{
   if (layer == nullptr)
   {
      return;
   }

   if (tileSet == nullptr)
   {
      return;
   }

   resetAll();

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

            const auto direction = static_cast<TileDirection>(tileNumber - firstId);
            sf::Vector2f directionVector;

            switch (direction)
            {
               case TileDirection::Up:
                  directionVector = vectorUp;
                  break;
               case TileDirection::Left:
                  directionVector = vectorLeft;
                  break;
               case TileDirection::Right:
                  directionVector = vectorRight;
                  break;
               case TileDirection::Down:
                  directionVector = vectorDown;
                  break;
            }

            auto tile = std::make_shared<FanTile>();

            const auto x = i * PIXELS_PER_TILE;
            const auto y = j * PIXELS_PER_TILE;

            tile->mPosition    = sf::Vector2i(i * PIXELS_PER_TILE, j * PIXELS_PER_TILE);
            tile->mRect.left   = x;
            tile->mRect.top    = y;
            tile->mRect.width  = PIXELS_PER_TILE;
            tile->mRect.height = PIXELS_PER_TILE;
            tile->mDir         = direction;
            tile->mDirection   = directionVector;
            sTiles.push_back(tile);

            createPhysics(world, tile);
         }
      }
   }
}


void Fan::resetAll()
{
    sFans.clear();
    sTiles.clear();
    sObjects.clear();
    sWeights.clear();
}


void Fan::addObject(TmxObject* object, const std::filesystem::path& basePath)
{
   sObjects.push_back(object);

   auto fan = std::make_shared<Fan>();
   sFans.push_back(fan);

   const auto w = static_cast<int32_t>(object->mWidth);
   const auto h = static_cast<int32_t>(object->mHeight);

   fan->mTexture = TexturePool::getInstance().get(basePath / "tilesets" / "fan.png");
   fan->mPixelRect.left = static_cast<int32_t>(object->mX);
   fan->mPixelRect.top = static_cast<int32_t>(object->mY);
   fan->mPixelRect.width = w;
   fan->mPixelRect.height = h;

   if (object->mProperties)
   {
       auto speedProp = object->mProperties->mMap["speed"];
       fan->mSpeed = speedProp ? speedProp->mValueFloat : 1.0f;
   }
}


std::optional<sf::Vector2f> Fan::collide(const sf::Rect<int32_t>& playerRect)
{
   // need to find all intersections since there can be more than one
   auto valid = false;
   sf::Vector2f dir;

   for (auto f : sFans)
   {
      auto fan = std::dynamic_pointer_cast<Fan>(f);

      if (!fan->isEnabled())
      {
         continue;
      }

      if (playerRect.intersects(fan->mPixelRect))
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


void Fan::collide(const sf::Rect<int32_t>& playerRect, b2Body* body)
{
   auto dir = collide(playerRect);
   if (dir.has_value())
   {
      body->ApplyForceToCenter(
         b2Vec2(2.0f * dir->x, -dir->y),
         true
      );
   }
}


void Fan::merge()
{
   auto xOffset = 0.0f;
   for (auto& tile : sTiles)
   {
      for (auto& f : sFans)
      {
         auto fan = std::dynamic_pointer_cast<Fan>(f);
         if (tile->mRect.intersects(fan->mPixelRect))
         {
            sf::Sprite sprite;
            sprite.setTexture(*fan->mTexture);
            sprite.setPosition(static_cast<float>(tile->mPosition.x), static_cast<float>(tile->mPosition.y));

            fan->mTiles.push_back(tile);
            fan->mDirection = tile->mDirection * fan->mSpeed;
            fan->mSprites.push_back(sprite);
            fan->mXOffsets.push_back(xOffset);
            fan->updateSprite();

            xOffset += 1.0f;
         }
      }
   }

   sTiles.clear();
}


Fan::FanTile::~FanTile()
{
   mBody->GetWorld()->DestroyBody(mBody);
}


