#include "tilemap.h"

#include <map>
#include <math.h>
#include <iostream>

// tmx
#include "player.h"
#include "tmxparser/tmxanimation.h"
#include "tmxparser/tmxframe.h"
#include "tmxparser/tmximage.h"
#include "tmxparser/tmxlayer.h"
#include "tmxparser/tmxtile.h"
#include "tmxparser/tmxtileset.h"
#include "tmxparser/tmxproperties.h"
#include "tmxparser/tmxproperty.h"


namespace
{
   static const int32_t blockSize = 128;
}


bool TileMap::isVisible() const
{
  return mVisible;
}


void TileMap::setVisible(bool visible)
{
  mVisible = visible;
}


bool TileMap::load(
    TmxLayer* layer,
    TmxTileSet* tileSet,
    const std::filesystem::path& basePath
)
{
   if (tileSet == nullptr)
   {
      // std::cout << "TileMap::load: tileSet is a nullptr" << std::endl;
      return false;
   }

   auto path = (basePath / tileSet->mImage->mSource).string();

   if (!mTexture.loadFromFile(path))
   {
      std::cout << "TileMap::load: can't load texture: " << path << std::endl;
      return false;
   }

   // std::cout << "TileMap::load: loading tileset: " << tileSet->mName << " with: texture " << path << std::endl;

   mTileSize    = sf::Vector2u(tileSet->mTileWidth, tileSet->mTileHeight);
   mVisible     = layer->mVisible;
   mZ           = layer->mZ;

   mVerticesStatic.setPrimitiveType(sf::Quads);
   mVerticesAnimated.setPrimitiveType(sf::Quads);

   std::map<int, TmxTile*> tileMap = tileSet->mTileMap;

   // populate the vertex array, with one quad per tile
   for (auto posX = 0u; posX < layer->mWidth; ++posX)
   {
      for (auto posY = 0u; posY < layer->mHeight; ++posY)
      {
         // get the current tile number
         auto tileNumber = layer->mData[posX + posY * layer->mWidth];

         if (tileNumber != 0)
         {
            // find its position in the tileset texture
            auto tu = (tileNumber - tileSet->mFirstGid) % (mTexture.getSize().x / mTileSize.x);
            auto tv = (tileNumber - tileSet->mFirstGid) / (mTexture.getSize().x / mTileSize.x);

            auto tx = posX + layer->mOffsetX;
            auto ty = posY + layer->mOffsetY;

            // define its 4 corners
            sf::Vertex quad[4];
            quad[0].position = sf::Vector2f(static_cast<float>( tx      * mTileSize.x), static_cast<float>( ty      * mTileSize.y));
            quad[1].position = sf::Vector2f(static_cast<float>((tx + 1) * mTileSize.x), static_cast<float>( ty      * mTileSize.y));
            quad[2].position = sf::Vector2f(static_cast<float>((tx + 1) * mTileSize.x), static_cast<float>((ty + 1) * mTileSize.y));
            quad[3].position = sf::Vector2f(static_cast<float>( tx      * mTileSize.x), static_cast<float>((ty + 1) * mTileSize.y));

            // define its 4 texture coordinates
            quad[0].texCoords = sf::Vector2f(static_cast<float>( tu      * mTileSize.x), static_cast<float>( tv      * mTileSize.y));
            quad[1].texCoords = sf::Vector2f(static_cast<float>((tu + 1) * mTileSize.x), static_cast<float>( tv      * mTileSize.y));
            quad[2].texCoords = sf::Vector2f(static_cast<float>((tu + 1) * mTileSize.x), static_cast<float>((tv + 1) * mTileSize.y));
            quad[3].texCoords = sf::Vector2f(static_cast<float>( tu      * mTileSize.x), static_cast<float>((tv + 1) * mTileSize.y));

            quad[0].color = sf::Color(255, 255, 255, static_cast<sf::Uint8>(layer->mOpacity * 255.0f));
            quad[1].color = sf::Color(255, 255, 255, static_cast<sf::Uint8>(layer->mOpacity * 255.0f));
            quad[2].color = sf::Color(255, 255, 255, static_cast<sf::Uint8>(layer->mOpacity * 255.0f));
            quad[3].color = sf::Color(255, 255, 255, static_cast<sf::Uint8>(layer->mOpacity * 255.0f));

            // build animation shader data
            std::map<int, TmxTile*>::iterator it = tileMap.find(tileNumber - tileSet->mFirstGid);
            if (it != tileMap.end() && it->second->mAnimation)
            {
               // only animated tiles are defined, non-animated tiles can be considered static tiles
               TmxAnimation* animation = it->second->mAnimation;

               std::vector<TmxFrame*> frames = animation->mFrames;

               AnimatedTile* animatedTile = new AnimatedTile();
               animatedTile->mTileX = tx;
               animatedTile->mTileY = ty;
               animatedTile->mAnimation = animation;

               float duration = 0.0f;
               for (auto frame : frames)
               {
                  // printf(
                  //    "animate: tile: %d, subst: %d, duration: %d \n",
                  //    tileNumber,
                  //    frame->mTileId,
                  //    frame->mDuration
                  // );

                  auto offsetFrame = new AnimatedTileFrame();
                  offsetFrame->mX = frame->mTileId % (mTexture.getSize().x / mTileSize.x);
                  offsetFrame->mY = frame->mTileId / (mTexture.getSize().x / mTileSize.x);
                  offsetFrame->mDuration = frame->mDuration;
                  animatedTile->mFrames.push_back(offsetFrame);
                  duration += frame->mDuration;
               }

               animatedTile->mDuration = duration;

               animatedTile->mVertices[0] = quad[0];
               animatedTile->mVertices[1] = quad[1];
               animatedTile->mVertices[2] = quad[2];
               animatedTile->mVertices[3] = quad[3];

               mAnimations.push_back(animatedTile);
            }
            else
            {
               // if no animation is available, just store the tile in the static buffer
               //int32_t blockId = (tx * blockWidth / blockSize + ty / blockSize);

               const auto bx = tx / blockSize;
               const auto by = ty / blockSize;

               auto yIt = mVerticesStaticBlocks.find(by);
               if (yIt == mVerticesStaticBlocks.end())
               {
                  std::map<int32_t, sf::VertexArray> map;
                  mVerticesStaticBlocks.insert(std::make_pair(by, map));
               }

               const auto xIt = mVerticesStaticBlocks[by].find(bx);
               if (xIt == mVerticesStaticBlocks[by].end())
               {
                  mVerticesStaticBlocks[by][bx].setPrimitiveType(sf::Quads);
               }

               mVerticesStaticBlocks[by][bx].append(quad[0]);
               mVerticesStaticBlocks[by][bx].append(quad[1]);
               mVerticesStaticBlocks[by][bx].append(quad[2]);
               mVerticesStaticBlocks[by][bx].append(quad[3]);

               // mVerticesStaticBlocks[blockId]->append(quad[0]);
               // mVerticesStaticBlocks[blockId]->append(quad[1]);
               // mVerticesStaticBlocks[blockId]->append(quad[2]);
               // mVerticesStaticBlocks[blockId]->append(quad[3]);
            }
         }
      }
   }

   return true;
}


void TileMap::update(const sf::Time& dt)
{
   mVerticesAnimated.clear();

   for (auto anim : mAnimations)
   {
      if (!anim->mVisible)
         continue;

      anim->mElapsed += dt.asMilliseconds();
      anim->mElapsed = fmod(anim->mElapsed, anim->mDuration);

      int index = 0;
      float frameDuration = 0.0f;
      for (auto frame : anim->mFrames)
      {
         frameDuration += frame->mDuration;

         if (frameDuration > anim->mElapsed)
         {
            break;
         }
         else
         {
            index++;
         }
      }

      AnimatedTileFrame* frame = anim->mFrames.at(index);

      int tu = frame->mX;
      int tv = frame->mY;

      // re-define its 4 texture coordinates
      anim->mVertices[0].texCoords = sf::Vector2f(static_cast<float>( tu      * mTileSize.x), static_cast<float>( tv      * mTileSize.y));
      anim->mVertices[1].texCoords = sf::Vector2f(static_cast<float>((tu + 1) * mTileSize.x), static_cast<float>( tv      * mTileSize.y));
      anim->mVertices[2].texCoords = sf::Vector2f(static_cast<float>((tu + 1) * mTileSize.x), static_cast<float>((tv + 1) * mTileSize.y));
      anim->mVertices[3].texCoords = sf::Vector2f(static_cast<float>( tu      * mTileSize.x), static_cast<float>((tv + 1) * mTileSize.y));

      mVerticesAnimated.append(anim->mVertices[0]);
      mVerticesAnimated.append(anim->mVertices[1]);
      mVerticesAnimated.append(anim->mVertices[2]);
      mVerticesAnimated.append(anim->mVertices[3]);
   }
}


void TileMap::draw(sf::RenderTarget &target, sf::RenderStates states) const
{
   if (!mVisible)
   {
      return;
   }

   // apply the transform
   states.transform *= getTransform();

   // apply the tileset texture
   states.texture = &mTexture;

   // draw the vertex arrays
   const auto pos = Player::getCurrent()->getPixelPositioni();

   int32_t bx = (pos.x / PIXELS_PER_TILE) / blockSize;
   int32_t by = (pos.y / PIXELS_PER_TILE) / blockSize;

   int32_t yRange = 1;
   int32_t xRange = 2;

   for (auto iy = by - yRange; iy < by + yRange; iy++)
   {
      auto yIt = mVerticesStaticBlocks.find(iy);
      if (yIt != mVerticesStaticBlocks.end())
      {
         for (auto ix = bx - xRange; ix < bx + xRange; ix++)
         {
            const auto xIt = mVerticesStaticBlocks[iy].find(ix);
            if (xIt != mVerticesStaticBlocks[iy].end())
            {
               target.draw(xIt->second, states);
            }
         }
      }
   }

   target.draw(mVerticesAnimated, states);
}


int TileMap::getZ() const
{
   return mZ;
}


void TileMap::setZ(int z)
{
   mZ = z;
}


void TileMap::hideTile(int x, int y, int vertexOffset)
{
   auto it =
      std::find_if(std::begin(mAnimations), std::end(mAnimations), [x, y](AnimatedTile* tile) {
            return (tile->mTileX == x && tile->mTileY == y);
         }
      );

   if (it != mAnimations.end())
   {
      // printf("setting animation at %d %d to invisible\n", (*it)->mTileX, (*it)->mTileX);
      (*it)->mVisible = false;
   }
   else
   {
      mVerticesStatic[vertexOffset * 4    ].color.a = 0;
      mVerticesStatic[vertexOffset * 4 + 1].color.a = 0;
      mVerticesStatic[vertexOffset * 4 + 2].color.a = 0;
      mVerticesStatic[vertexOffset * 4 + 3].color.a = 0;
   }
}

