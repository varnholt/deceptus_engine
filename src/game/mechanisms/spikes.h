#pragma once

#include "gamemechanism.h"
#include "gamenode.h"

#include "SFML/Graphics.hpp"

#include <filesystem>
#include <Box2D/Box2D.h>

struct TmxLayer;
struct TmxTileSet;

class Spikes : public GameMechanism, public GameNode
{
public:

   enum class Mode
   {
      Invalid,
      Trap,
      Interval,
      Toggled,
   };

   enum class Orientation
   {
      Invalid,
      PointsTop,
      PointsLeft,
      PointsUp,
      PointsDown
   };

   Spikes(GameNode* parent = nullptr);

   void draw(sf::RenderTarget& window) override;
   void update(const sf::Time& dt) override;

   static std::vector<std::shared_ptr<Spikes>> load(
      TmxLayer *layer,
      TmxTileSet *tileSet,
      const std::filesystem::path& basePath,
      Mode mode
   );

   const sf::IntRect& getPixelRect() const;

   Mode getMode() const;
   void setMode(Mode mode);


private:

   void updateInterval();
   void updateTrap();
   void updateToggled();

   sf::Vector2u mTileSize;

   std::shared_ptr<sf::Texture> mTexture;

   int32_t mTu = 0;
   int32_t mTv = 0;

   sf::Sprite mSprite;
   int32_t mElapsedMs = 0;

   sf::Vector2f mTilePosition;
   sf::Vector2f mPixelPosition;
   sf::IntRect mPixelRect;

   bool mTriggered = false;
   bool mDeadly = false;

   Mode mMode = Mode::Invalid;
   Orientation mOrientation = Orientation::Invalid;
   void updateSpriteRect();
};

