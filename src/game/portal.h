#ifndef PORTAL_H
#define PORTAL_H

// sfml
#include "SFML/Graphics.hpp"

// box2d
#include "Box2D/Box2D.h"

// std
#include <filesystem>

struct TmxLayer;
struct TmxObject;
struct TmxTileSet;



class Portal
{

   public:

   Portal() = default;

   void draw(sf::RenderTarget& window);
   void update(const sf::Time& dt);

   static std::vector<std::shared_ptr<Portal>> load(
      TmxLayer *layer,
      TmxTileSet *tileSet,
      const std::filesystem::path& basePath,
      const std::shared_ptr<b2World>& world
   );

   static void link(std::vector<std::shared_ptr<Portal>>& portals, TmxObject*);

   bool isPlayerAtPortal() const;
   void setPlayerAtPortal(bool isPlayerAtPortal);

   void addSprite(const sf::Sprite&);

   int getZ() const;
   void setZ(int z);

   std::shared_ptr<Portal> getDestination() const;
   void setDestination(const std::shared_ptr<Portal>& dst);

   sf::Vector2f getPortalPosition();
   const sf::Vector2f& getTilePosition() const;


protected:

   sf::Vector2u mTileSize;
   sf::Texture mTexture;

   std::vector<sf::Sprite> mSprites;

   sf::Vector2f mTilePosition;

   int mHeight = 0;
   bool mPlayerAtPortal = false;
   int mZ = 0;
   std::shared_ptr<Portal> mDestination;
};

#endif // PORTAL_H
