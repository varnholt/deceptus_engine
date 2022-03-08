#pragma once

#include "gamemechanism.h"
#include "gamenode.h"

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

class Laser : public GameMechanism, public GameNode
{
public:

   struct Signal
   {
      uint32_t mDurationMs = 0u;
      bool mOn = false;
   };

   Laser(GameNode* parent = nullptr);

   void draw(sf::RenderTarget& color, sf::RenderTarget& normal) override;
   void update(const sf::Time& dt) override;

   static std::vector<std::shared_ptr<GameMechanism>> load(
      GameNode* parent,
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

   static std::vector<TmxObject*> __objects;
   static std::vector<std::shared_ptr<Laser>> __lasers;
   static std::vector<std::array<int32_t, 9>> __tiles_version_1;
   static std::vector<std::array<int32_t, 9>> __tiles_version_2;

   std::vector<Signal> _signal_plot;

   int32_t _tu = 0;
   int32_t _tv = 0;

   std::shared_ptr<sf::Texture> _texture;
   sf::Sprite _sprite;

   sf::Vector2f _tile_position;
   sf::Vector2f _pixel_position;
   sf::Rect<int32_t> _pixel_rect;

   bool _on = true;
   int32_t _tile_index = 0;
   float _tile_animation = 0.0f;
   int32_t _animation_offset = 0;
   uint32_t _signal_index = 0;
   uint32_t _time = 0u;
   int32_t _group_id = 0; // only for debugging purposes
};

