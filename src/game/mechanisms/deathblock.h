#pragma once

#include "constants.h"
#include "framework/math/pathinterpolation.h"
#include "gamemechanism.h"
#include "gamenode.h"

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

      enum SpikeOrientation
      {
         Up     = 0,
         Left   = 1,
         Center = 2,
         Right  = 3,
         Down   = 4
      };

      void setupTransform();
      void setupBody(const std::shared_ptr<b2World>& world);
      void updateLeverLag(const sf::Time& dt);
      void updateCollision();

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

      std::array<sf::IntRect, 4> mCollisionRects = {
          sf::IntRect{1 * PIXELS_PER_TILE, 0 * PIXELS_PER_TILE, PIXELS_PER_TILE, PIXELS_PER_TILE},
          sf::IntRect{0 * PIXELS_PER_TILE, 1 * PIXELS_PER_TILE, PIXELS_PER_TILE, PIXELS_PER_TILE},
          sf::IntRect{1 * PIXELS_PER_TILE, 2 * PIXELS_PER_TILE, PIXELS_PER_TILE, PIXELS_PER_TILE},
          sf::IntRect{2 * PIXELS_PER_TILE, 1 * PIXELS_PER_TILE, PIXELS_PER_TILE, PIXELS_PER_TILE}
      };

      int32_t mElapsedMs = 0;

      sf::Vector2f mPixelPosition;
      std::vector<sf::Vector2f> mPixelPath;

      b2Body* mBody = nullptr;
      std::vector<b2Vec2> mPath;
      PathInterpolation mInterpolation;
      float mLeverLag = 1.0f; // maybe make them switchable as well?

      std::shared_ptr<sf::Texture> mTexture;
};

