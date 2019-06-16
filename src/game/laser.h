#pragma once

// sfml
#include "SFML/Graphics.hpp"

// box2d
#include "Box2D/Box2D.h"

// std
#include <filesystem>

struct TmxLayer;
struct TmxObject;
struct TmxTileSet;

class Laser
{
public:

   struct Signal
   {
      int32_t mDurationMs = 0;
      bool mOn = false;
   };

   Laser() = default;

   void draw(sf::RenderTarget& window);
   void update(float dt);

   static std::vector<Laser*> load(
      TmxLayer *layer,
      TmxTileSet *tileSet,
      const std::filesystem::path& basePath,
      const std::shared_ptr<b2World>& world
   );

   int getZ() const;
   void setZ(int z);


protected:

   std::vector<Signal> mSignalPlot;

   sf::Vector2u mTileSize;
   sf::Texture mTexture;
   int32_t mTu = 0;
   int32_t mTv = 0;

   sf::Sprite mSprite;

   sf::Vector2f mTilePosition;

   int mZ = 0;
};

