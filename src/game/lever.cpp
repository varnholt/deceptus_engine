#include "lever.h"

#include "constants.h"
#include "gamemechanism.h"
#include "laser.h"
#include "player.h"

#include "tmxparser/tmxlayer.h"
#include "tmxparser/tmxobject.h"
#include "tmxparser/tmxtileset.h"

#include <iostream>


std::vector<TmxObject*> Lever::mRectangles;
std::vector<std::shared_ptr<Lever>> Lever::sLevers;


//-----------------------------------------------------------------------------
std::vector<std::shared_ptr<GameMechanism>> Lever::load(
   TmxLayer* layer,
   TmxTileSet* tileSet,
   const std::filesystem::path& /*basePath*/,
   const std::shared_ptr<b2World>&
)
{
   std::vector<std::shared_ptr<GameMechanism>> levers;

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

            if (tileId == 33)
            {
               std::cout << "lever at " << i << ", " << j << std::endl;

               auto lever = std::make_shared<Lever>();
               lever->mRect.left = PIXELS_PER_TILE * i;
               lever->mRect.top = PIXELS_PER_TILE * j;
               lever->mRect.width = PIXELS_PER_TILE * 3;
               lever->mRect.height = PIXELS_PER_TILE;

               sLevers.push_back(lever);
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
void Lever::update(const sf::Time& /*dt*/)
{
   auto playerRect = Player::getCurrent()->getPlayerPixelRect();
   setPlayerAtLever(mRect.intersects(playerRect));
}


//-----------------------------------------------------------------------------
void Lever::toggle()
{
   if (mType == Type::TwoState)
   {
      switch (mState)
      {
         case State::Left:
            mState = State::Right;
            break;
         case State::Right:
            mState = State::Left;
            break;
         case State::Middle:
            break;
      }
   }

   else if (mType == Type::TriState)
   {
      switch (mState)
      {
         case State::Left:
         {
            mState = State::Middle;
            break;
         }
         case State::Middle:
         {
            if (mPreviousState == State::Left)
            {
               mState = State::Right;
            }
            else
            {
               mState = State::Left;
            }
            break;
         }
         case State::Right:
         {
            mState = State::Middle;
            break;
         }
      }

      mPreviousState = mState;
   }

   for (auto& cb : mCallbacks)
   {
      cb(static_cast<int32_t>(mState));
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
   std::vector<std::shared_ptr<GameMechanism>> lasers,
   std::vector<std::shared_ptr<GameMechanism>> fans,
   std::vector<std::shared_ptr<GameMechanism>> platforms,
   std::vector<std::shared_ptr<GameMechanism>> belts
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
                  callbacks.push_back([&](int32_t state) {
                        laser->setEnabled(state == -1 ? true : false);
                     }
                  );
                  std::cout << "found laser in search rect" << std::endl;
               }
            }

            lever->setCallbacks(callbacks);

            break;
         }
      }
   }
}


