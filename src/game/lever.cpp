#include "lever.h"

#include "constants.h"
#include "conveyorbelt.h"
#include "gamemechanism.h"
#include "fan.h"
#include "laser.h"
#include "movingplatform.h"
#include "player.h"
#include "texturepool.h"

#include "tmxparser/tmxlayer.h"
#include "tmxparser/tmxobject.h"
#include "tmxparser/tmxtileset.h"

#include <iostream>


sf::Texture Lever::sTexture;

std::vector<TmxObject*> Lever::mRectangles;
std::vector<std::shared_ptr<Lever>> Lever::sLevers;


namespace
{
static const auto SPRITES_PER_ROW = 11;
static const auto ROW_CENTER = 6;
}


//-----------------------------------------------------------------------------
std::vector<std::shared_ptr<GameMechanism>> Lever::load(
   TmxLayer* layer,
   TmxTileSet* tileSet,
   const std::filesystem::path& basePath,
   const std::shared_ptr<b2World>&
)
{
   sLevers.clear();
   sTexture = *TexturePool::getInstance().get(basePath / "tilesets" / "levers.png");

   std::vector<std::shared_ptr<GameMechanism>> levers;

   // auto tilesize = sf::Vector2u(static_cast<uint32_t>(tileSet->mTileWidth), static_cast<uint32_t>(tileSet->mTileHeight));
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

            if (tileId == 33)
            {
               std::cout << "lever at " << i << ", " << j << std::endl;

               auto lever = std::make_shared<Lever>();

               // sprite is two tiles high

               const auto x = PIXELS_PER_TILE * i;
               const auto y = PIXELS_PER_TILE * (j - 1);

               lever->mRect.left = x;
               lever->mRect.top = y;
               lever->mRect.width = PIXELS_PER_TILE * 3;
               lever->mRect.height = PIXELS_PER_TILE * 2;

               lever->mSprite.setPosition(x, y);
               lever->mSprite.setTexture(sTexture);
               lever->updateSprite();

               sLevers.push_back(lever);
               levers.push_back(lever);
            }
         }
      }
   }

   return levers;
}


//-----------------------------------------------------------------------------
bool Lever::getPlayerAtLever() const
{
   return mPlayerAtLever;
}


//-----------------------------------------------------------------------------
void Lever::setPlayerAtLever(bool playerAtLever)
{
   mPlayerAtLever = playerAtLever;
}


//-----------------------------------------------------------------------------
void Lever::updateSprite()
{
   mSprite.setTextureRect({
      mOffset * 3 * PIXELS_PER_TILE,
      mDir == -1 ? (2 * PIXELS_PER_TILE) : 0,
      PIXELS_PER_TILE * 3,
      PIXELS_PER_TILE * 2
   });
}


//-----------------------------------------------------------------------------
void Lever::update(const sf::Time& /*dt*/)
{
   auto playerRect = Player::getCurrent()->getPlayerPixelRect();
   setPlayerAtLever(mRect.intersects(playerRect));

   bool reached = false;

   if (mTargetState == State::Left)
   {
      mDir = -1;
      reached = (mOffset == 0);
   }
   else if (mTargetState == State::Right)
   {
      mDir = 1;
      reached = (mOffset == SPRITES_PER_ROW - 1);
   }
   else if (mTargetState == State::Middle)
   {
      mDir = (mPreviousState == State::Left) ? 1 : -1;
      reached = (mOffset == ROW_CENTER);
   };

   if (reached)
   {
      return;
   }

   mOffset += mDir;

   updateSprite();
}


//-----------------------------------------------------------------------------
void Lever::draw(sf::RenderTarget& target)
{
   target.draw(mSprite);
}


//-----------------------------------------------------------------------------
void Lever::toggle()
{
   if (!mPlayerAtLever)
   {
      return;
   }

   if (mType == Type::TwoState)
   {
      switch (mTargetState)
      {
         case State::Left:
            mTargetState = State::Right;
            break;
         case State::Right:
            mTargetState = State::Left;
            break;
         case State::Middle:
            break;
      }
   }

   else if (mType == Type::TriState)
   {
      switch (mTargetState)
      {
         case State::Left:
         {
            mTargetState = State::Middle;
            break;
         }
         case State::Middle:
         {
            if (mPreviousState == State::Left)
            {
               mTargetState = State::Right;
            }
            else
            {
               mTargetState = State::Left;
            }
            break;
         }
         case State::Right:
         {
            mTargetState = State::Middle;
            break;
         }
      }

      mPreviousState = mTargetState;
   }

   for (auto& cb : mCallbacks)
   {
      cb(static_cast<int32_t>(mTargetState));
   }
}


//-----------------------------------------------------------------------------
void Lever::setCallbacks(const std::vector<Callback>& callbacks)
{
   mCallbacks = callbacks;
}


//-----------------------------------------------------------------------------
void Lever::addSearchRect(TmxObject* rect)
{
   mRectangles.push_back(rect);
}


//-----------------------------------------------------------------------------
void Lever::merge(
   const std::vector<std::shared_ptr<GameMechanism>>& lasers,
   const std::vector<std::shared_ptr<GameMechanism>>& platforms,
   const std::vector<std::shared_ptr<GameMechanism>>& fans,
   const std::vector<std::shared_ptr<GameMechanism>>& belts
)
{
   for (auto rect : mRectangles)
   {
      sf::Rect<int32_t> searchRect;
      searchRect.left = static_cast<int32_t>(rect->mX);
      searchRect.top = static_cast<int32_t>(rect->mY);
      searchRect.width = static_cast<int32_t>(rect->mWidth);
      searchRect.height = static_cast<int32_t>(rect->mHeight);

      // std::cout
      //    << "x: " << searchRect.left << " "
      //    << "y: " << searchRect.top << " "
      //    << "w: " << searchRect.width << " "
      //    << "h: " << searchRect.height << " "
      //    << std::endl;

      for (auto& lever : sLevers)
      {
         if (lever->mRect.intersects(searchRect))
         {
            // std::cout << "found match" << std::endl;

            std::vector<Callback> callbacks;

            for (auto l : lasers)
            {
               auto laser = std::dynamic_pointer_cast<Laser>(l);

               if (laser->getPixelRect().intersects(searchRect))
               {
                  callbacks.push_back([laser](int32_t state) {
                        laser->setEnabled(state == -1 ? true : false);
                     }
                  );
               }
            }

            for (auto b : belts)
            {
               auto belt = std::dynamic_pointer_cast<ConveyorBelt>(b);

               if (belt->getPixelRect().intersects(searchRect))
               {
                  callbacks.push_back([belt](int32_t state) {
                        belt->setEnabled(state == -1 ? true : false);
                     }
                  );
               }
            }

            for (auto f : fans)
            {
               auto fan = std::dynamic_pointer_cast<Fan>(f);

               if (fan->getPixelRect().intersects(searchRect))
               {
                  callbacks.push_back([fan](int32_t state) {
                        fan->setEnabled(state == -1 ? true : false);
                     }
                  );
               }
            }

            for (auto p : platforms)
            {
               auto platform = std::dynamic_pointer_cast<MovingPlatform>(p);

               const auto& pixelPath = platform->getPixelPath();

               for (const auto& pixel : pixelPath)
               {
                  if (searchRect.contains(static_cast<int32_t>(pixel.x), static_cast<int32_t>(pixel.y)))
                  {
                     callbacks.push_back([platform](int32_t state) {
                           platform->setEnabled(state == -1 ? true : false);
                        }
                     );

                     break;
                  }
               }

            }

            lever->setCallbacks(callbacks);

            break;
         }
      }
   }
}


