#pragma once

#include "gamemechanism.h"

// sfml
#include "SFML/Graphics.hpp"

// box2d
#include "Box2D/Box2D.h"

// std
#include <array>
#include <filesystem>
#include <vector>


struct TmxLayer;
struct TmxObject;
struct TmxTileSet;

class Laser : public GameMechanism
{
public:

   struct Signal
   {
      uint32_t mDurationMs = 0u;
      bool mOn = false;
   };

   enum class MechanismVersion
   {
      Version1,
      Version2
   };

   Laser() = default;

   void draw(sf::RenderTarget& window) override;
   void update(const sf::Time& dt) override;

   static std::vector<std::shared_ptr<GameMechanism>> load(
      TmxLayer *layer,
      TmxTileSet *tileSet,
      const std::filesystem::path& basePath,
      const std::shared_ptr<b2World>& world
   );

   static void addObject(TmxObject* object);
   static void addTilesVersion1();
   static void addTilesVersion2();
   static void merge();
   static void collide(const sf::Rect<int32_t>& playerRect);

   void reset();
   static void resetAll();

   const sf::Vector2f& getTilePosition() const;
   const sf::Vector2f& getPixelPosition() const;
   const sf::Rect<int32_t>& getPixelRect() const;

   void setEnabled(bool enabled) override;


protected:

   static std::vector<TmxObject*> mObjects;
   static std::vector<std::shared_ptr<Laser>> mLasers;
   static std::vector<std::array<int32_t, 9>> mTilesVersion1;
   static std::vector<std::array<int32_t, 9>> mTilesVersion2;

   std::vector<Signal> mSignalPlot;

   int32_t mTu = 0;
   int32_t mTv = 0;

   std::shared_ptr<sf::Texture> mTexture;
   sf::Sprite mSprite;

   sf::Vector2f mTilePosition;
   sf::Vector2f mPixelPosition;
   sf::Rect<int32_t> mPixelRect;

   bool mOn = true;
   int32_t mTileIndex = 0;
   float mTileAnimation = 0.0f;
   int32_t mAnimationOffset = 0;
   uint32_t mSignalIndex = 0;
   uint32_t mTime = 0u;
   int32_t mGroupId = 0; // only for debugging purposes

   MechanismVersion mVersion = MechanismVersion::Version1;
};

