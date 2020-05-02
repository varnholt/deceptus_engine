#pragma once

#include "gamemechanism.h"
#include "gamenode.h"
#include "pathinterpolation.h"

#include <Box2D/Box2D.h>

#include <array>
#include <filesystem>

struct TmxLayer;
struct TmxObject;
struct TmxTileSet;


class DeathBlock : public GameMechanism, public GameNode
{
   public:

      enum class Mode
      {
         Invalid,
         AlwaysOn,
         OnContact,
         Interval,
      };

      DeathBlock(GameNode* parent = nullptr);

      void draw(sf::RenderTarget& target) override;
      void update(const sf::Time& dt) override;

      void setup(
         TmxObject* tmxObject,
         const std::shared_ptr<b2World>& world
      );


private:

   void setupTransform();
   void setupBody(const std::shared_ptr<b2World>& world);

   int32_t mTu = 0;
   int32_t mTv = 0;

   //     +---+
   //     | 0 |
   // +---+---+---+
   // | 1 | 2 | 3 |
   // +---+---+---+
   //     | 4 |
   //     +---+

   std::array<sf::Sprite, 5> mSprites;
   std::array<int32_t, 5> mStates = {};
   std::array<sf::Vector2i, 5> mOffsets = {
      sf::Vector2i{1, 0},
      sf::Vector2i{0, 1},
      sf::Vector2i{1, 1},
      sf::Vector2i{1, 2},
      sf::Vector2i{2, 1}
   };

   int32_t mElapsedMs = 0;

   sf::Vector2f mTilePosition;
   sf::Vector2f mPixelPosition;
   sf::IntRect mPixelRect;
   std::vector<sf::Vector2f> mPixelPath;

   b2Body* mBody = nullptr;
   std::vector<b2Vec2> mPath;
   PathInterpolation mInterpolation;
   float mLeverLag = 1.0f; // maybe make them switchable as well?

   static sf::Texture sTexture;
};

