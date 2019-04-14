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

   Portal();

   void draw(sf::RenderTarget& window);
   void update(float dt);

   static std::vector<Portal*> load(
      TmxLayer *layer,
      TmxTileSet *tileSet,
      const std::filesystem::path& basePath,
      const std::shared_ptr<b2World>& world
   );

   static void link(std::vector<Portal*>& portals, TmxObject*);

   bool isPlayerAtPortal() const;
   void setPlayerAtPortal(bool isPlayerAtPortal);

   void addSprite(const sf::Sprite&);

   int getZ() const;
   void setZ(int z);

   Portal *getDst() const;
   void setDst(Portal *dst);

   sf::Vector2f getPortalPosition();


protected:

   sf::Vector2u mTileSize;
   sf::Texture mTexture;

   std::vector<sf::Sprite> mSprites;

   sf::Vector2f mTilePosition;

   int mHeight;
   bool mPlayerAtPortal;
   int mZ;
   Portal* mDst;
};

#endif // PORTAL_H
