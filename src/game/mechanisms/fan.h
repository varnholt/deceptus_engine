#pragma once

#include <Box2D/Box2D.h>
#include <SFML/Graphics.hpp>

#include <filesystem>
#include <optional>
#include <memory>

#include "gamemechanism.h"

struct TmxLayer;
struct TmxObject;
struct TmxTileSet;


class Fan : public GameMechanism
{
   public:

      enum class TileDirection
      {
         Up    =  0,
         Right =  8,
         Left  = 16,
         Down  = 24,
      };

      struct FanTile
      {
         sf::Vector2i mPosition;
         sf::Vector2f mDirection;
         sf::Rect<int32_t> mRect;
         b2Body* mBody = nullptr;
         TileDirection mDir;

         ~FanTile();
      };

      Fan() = default;

      void draw(sf::RenderTarget& target) override;
      void update(const sf::Time& dt) override;
      const sf::Rect<int32_t>& getPixelRect() const;
      void setEnabled(bool enabled) override;

      static void load(
         TmxLayer* layer,
         TmxTileSet* tileSet,
         const std::shared_ptr<b2World>& world
      );

      static void resetAll();
      static void addObject(TmxObject* object, const std::filesystem::path& basePath);
      static std::optional<sf::Vector2f> collide(const sf::Rect<int32_t>& playerRect);
      static void collide(const sf::Rect<int32_t>& playerRect, b2Body* body);
      static void merge();

      static std::vector<std::shared_ptr<GameMechanism>>& getFans();


   private:

      void updateSprite();

      static void createPhysics(const std::shared_ptr<b2World>& world, const std::shared_ptr<FanTile>& item);

      static std::vector<std::shared_ptr<GameMechanism>> sFans;
      static std::vector<std::shared_ptr<FanTile>> sTiles;
      static std::vector<TmxObject*> sObjects;
      static std::vector<sf::Vector2f> sWeights;

      std::vector<std::shared_ptr<FanTile>> mTiles;

      uint32_t mWidth;
      uint32_t mHeight;
      sf::Vector2f mDirection;
      sf::Rect<int32_t> mPixelRect;
      float mSpeed = 1.0f;
      float mLeverLag = 1.0f;

      std::vector<sf::Sprite> mSprites;
      std::vector<float> mXOffsets;

      std::shared_ptr<sf::Texture> mTexture;
};

