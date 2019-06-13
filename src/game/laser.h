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

   Laser() = default;

   void draw(sf::RenderTarget& window);
   void update(float dt);

   static std::vector<Laser*> load(
      TmxLayer *layer,
      TmxTileSet *tileSet,
      const std::filesystem::path& basePath,
      const std::shared_ptr<b2World>& world
   );

   void addSprite(const sf::Sprite&);

   int getZ() const;
   void setZ(int z);


protected:

   sf::Vector2u mTileSize;
   sf::Texture mTexture;

   std::vector<sf::Sprite> mSprites;

   sf::Vector2f mTilePosition;

   int mZ = 0;
};

