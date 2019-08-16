#pragma once

// sfml
#include "SFML/Graphics.hpp"

// box2d
#include "Box2D/Box2D.h"

// std
#include <array>
#include <filesystem>
#include <vector>


struct TmxLayer;
struct TmxObject;
struct TmxTileSet;

class Laser
{
public:

   struct Signal
   {
      uint32_t mDurationMs = 0u;
      bool mOn = false;
   };

   Laser() = default;

   void draw(sf::RenderTarget& window);
   void update(const sf::Time& dt);

   static std::vector<Laser*> load(
      TmxLayer *layer,
      TmxTileSet *tileSet,
      const std::filesystem::path& basePath,
      const std::shared_ptr<b2World>& world
   );

   static void addObject(TmxObject* object);
   static void addTiles();
   static void merge();
   static void collide(const sf::Rect<int32_t>& playerRect);

   int getZ() const;
   void setZ(int z);


protected:

   static std::vector<TmxObject*> mObjects;
   static std::vector<Laser*> mLasers;
   static std::vector<std::array<int32_t, 9>> mTiles;

   std::vector<Signal> mSignalPlot;

   int32_t mTu = 0;
   int32_t mTv = 0;

   std::shared_ptr<sf::Texture> mTexture;
   sf::Sprite mSprite;

   sf::Vector2f mTilePosition;

   int32_t mZ = 0;

   bool mOn = false;
   int32_t mTileIndex = 0;
   float mTileAnimation = 0.0f;
   uint32_t mSignalIndex = 0;
   uint32_t mTime = 0u;
};

