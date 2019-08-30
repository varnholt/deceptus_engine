#pragma once

#include "constants.h"
#include "gamenode.h"

#include <filesystem>

#include "SFML/Graphics.hpp"

#include "Box2D/Box2D.h"


struct TmxLayer;
struct TmxTileSet;

class Door : public GameNode
{
public:

   enum class State
   {
      Open,
      Opening,
      Closing,
      Closed
   };

   Door(GameNode *parent);

   void draw(sf::RenderTarget& window);
   void update(const sf::Time& dt);
   void setupBody(const std::shared_ptr<b2World>& world);
   void addSprite(const sf::Sprite&);

   void toggle();

   static std::vector<Door*> load(
      TmxLayer *layer,
      TmxTileSet *tileSet,
      const std::filesystem::path& basePath,
      const std::shared_ptr<b2World>&
   );


   bool isPlayerAtDoor() const;
   void setPlayerAtDoor(bool isPlayerAtDoor);


   int getZ() const;
   void setZ(int z);

   void updateTransform();

   void reset();

   const sf::Vector2i& getTilePosition() const;


protected:

   void open();
   void close();

   sf::Vector2u mTileSize;
   sf::Texture mTexture;

   std::vector<sf::Sprite> mSprites;

   State mInitialState = State::Closed;
   State mState = State::Closed;

   sf::Vector2i mTilePosition;

   ItemType mRequiredItem = ItemType::Invalid;

   float mOffset = 0.0f;
   int mHeight = 0;
   int mTileId = 0;
   bool mPlayerAtDoor = false;
   int mZ = 0;
   b2Body* mBody = nullptr;
};

