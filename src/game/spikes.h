#pragma once

#include "SFML/Graphics.hpp"

#include <filesystem>
#include <Box2D/Box2D.h>

struct TmxLayer;
struct TmxTileSet;

class Spikes
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

   Spikes() = default;

   void draw(sf::RenderTarget& window);
   void update(const sf::Time& dt);

   static std::vector<std::shared_ptr<Spikes>> load(
      TmxLayer *layer,
      TmxTileSet *tileSet,
      const std::filesystem::path& basePath,
      const std::shared_ptr<b2World>& world
   );

   int32_t getZ() const;
   void setZ(const int32_t& z);


private:

   sf::Vector2u mTileSize;

   static sf::Texture mTexture;

   int32_t mTu = 0;
   int32_t mTv = 0;

   sf::Sprite mSprite;
   sf::Time mElapsed;

   sf::Vector2f mTilePosition;
   sf::Vector2f mPixelPosition;

   int32_t mZ = 0;

   bool mTriggered = false;
   bool mDeadly = false;

   Mode mMode = Mode::Invalid;
   Orientation mOrientation = Orientation::Invalid;
   void updateInterval();
};

