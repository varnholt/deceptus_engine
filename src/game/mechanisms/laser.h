#pragma once

#include "framework/math/pathinterpolation.h"
#include "game/gamedeserializedata.h"
#include "game/gamemechanism.h"
#include "game/gamenode.h"

// sfml
#include "SFML/Graphics.hpp"

// box2d
#include "Box2D/Box2D.h"

// std
#include <array>
#include <filesystem>
#include <optional>
#include <vector>


struct TmxLayer;
struct TmxObject;
struct TmxTileSet;

class Laser : public GameMechanism, public GameNode
{
public:

   struct Signal
   {
      uint32_t _duration_ms = 0u;
      bool _on = false;
   };

   Laser(GameNode* parent = nullptr);

   void draw(sf::RenderTarget& color, sf::RenderTarget& normal) override;
   void update(const sf::Time& dt) override;

   static std::vector<std::shared_ptr<GameMechanism>> load(GameNode* parent, const GameDeserializeData& data);

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

   std::vector<Signal> _signal_plot;

   int32_t _tu = 0;
   int32_t _tv = 0;

   std::shared_ptr<sf::Texture> _texture;
   sf::Sprite _sprite;

   sf::Vector2f _tile_position;
   sf::Vector2f _position_px;
   sf::Rect<int32_t> _pixel_rect;

   std::optional<std::vector<sf::Vector2f>> _path;
   sf::Vector2f _move_offset_px;
   PathInterpolation<sf::Vector2f> _path_interpolation;

   bool _on = true;
   int32_t _tile_index = 0;
   float _tile_animation = 0.0f;
   int32_t _animation_offset = 0;
   uint32_t _signal_index = 0;
   uint32_t _time = 0u;
   int32_t _group_id = 0; // only for debugging purposes
};

