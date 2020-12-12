#pragma once

#include "constants.h"
#include "gamemechanism.h"
#include "gamenode.h"

#include <filesystem>

#include "SFML/Graphics.hpp"

#include "Box2D/Box2D.h"


struct TmxLayer;
struct TmxTileSet;

class Door : public GameMechanism, public GameNode
{
public:

   enum class Type
   {
      Bars,
      Conventional,
   };

   enum class State
   {
      Open,
      Opening,
      Closing,
      Closed,
   };

   Door(GameNode *parent);

   void draw(sf::RenderTarget& window) override;
   void update(const sf::Time& dt) override;

   void toggle();

   static std::vector<std::shared_ptr<GameMechanism>> load(
      TmxLayer *layer,
      TmxTileSet *tileSet,
      const std::filesystem::path& basePath,
      const std::shared_ptr<b2World>&
   );

   bool isPlayerAtDoor() const;
   void setPlayerAtDoor(bool isPlayerAtDoor);

   void reset();

   const sf::Vector2i& getTilePosition() const;


private:

   void setupBody(
      const std::shared_ptr<b2World>& world,
      float xOffset = 0.0f,
      float xScale = 1.0f
   );

   void updateTransform();

   void updateBars(const sf::Time& dt);

   void open();
   void close();
   bool checkPlayerAtDoor() const;

   sf::Vector2u mTileSize;
   std::shared_ptr<sf::Texture> mTexture;

   sf::VertexArray mDoorQuad{sf::Quads, 4};
   sf::Sprite mSpriteIcon;

   Type mType = Type::Bars;

   State mInitialState = State::Closed;
   State mState = State::Closed;

   sf::Vector2i mTilePosition;

   ItemType mRequiredItem = ItemType::Invalid;

   float mOffset = 0.0f;
   int32_t mHeight = 0;
   int32_t mTileId = 0;
   bool mPlayerAtDoor = false;
   b2Body* mBody = nullptr;
};

