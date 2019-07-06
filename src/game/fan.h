#pragma once

#include <Box2D/Box2D.h>
#include <SFML/Graphics.hpp>

#include <filesystem>
#include <optional>
#include <memory>

struct TmxLayer;
struct TmxObject;
struct TmxTileSet;


class Fan
{
   public:


      struct FanTile
      {
         sf::Vector2i mPosition;
         sf::Vector2f mDirection;
         sf::Rect<int32_t> mRect;
         b2Body* mBody = nullptr;

         ~FanTile();
      };

      enum class DirectionTile
      {
         Up = 0,
         Right = 8,
         Left = 16,
      };

      Fan() = default;

      static void load(
         TmxLayer* layer,
         TmxTileSet* tileSet,
         const std::shared_ptr<b2World>& world
      );

      static void addObject(TmxObject* object);
      static std::optional<sf::Vector2f> collide(const sf::Rect<int32_t>& playerRect);
      static void merge();


   private:

      static void createPhysics(const std::shared_ptr<b2World>& world, FanTile* item);

      static std::vector<Fan*> sFans;
      static std::vector<FanTile*> sTiles;
      static std::vector<TmxObject*> sObjects;
      static std::vector<sf::Vector2f> sWeights;

      std::vector<FanTile*> mTiles;

      uint32_t mWidth;
      uint32_t mHeight;
      sf::Vector2f mDirection;
      sf::Rect<int32_t> mRect;
};

