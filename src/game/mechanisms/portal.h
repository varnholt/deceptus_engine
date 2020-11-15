#pragma once

#include "gamemechanism.h"
#include "gamenode.h"

// sfml
#include "SFML/Graphics.hpp"

// box2d
#include "Box2D/Box2D.h"

// std
#include <filesystem>

struct TmxLayer;
struct TmxObject;
struct TmxTileSet;



class Portal : public GameMechanism, public GameNode
{

public:

   Portal(GameNode* parent = nullptr);

   void draw(sf::RenderTarget& window) override;
   void update(const sf::Time& dt) override;

   static std::vector<std::shared_ptr<GameMechanism>> load(
      TmxLayer *layer,
      TmxTileSet *tileSet,
      const std::filesystem::path& basePath,
      const std::shared_ptr<b2World>& world
   );

   static void link(std::vector<std::shared_ptr<GameMechanism>>& portals, TmxObject*);

   bool isPlayerAtPortal() const;
   void setPlayerAtPortal(bool isPlayerAtPortal);

   void addSprite(const sf::Sprite&);

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
   std::shared_ptr<Portal> mDestination;
};

