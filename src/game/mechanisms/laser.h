#pragma once

#include "framework/math/pathinterpolation.h"
#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

// sfml
#include "SFML/Graphics.hpp"

// box2d
#include <box2d/box2d.h>

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

   struct Settings
   {
      float _movement_speed = 0.2f;
   };

   Laser(GameNode* parent = nullptr);
   std::string_view objectName() const override;

   void draw(sf::RenderTarget& color, sf::RenderTarget& normal) override;
   void update(const sf::Time& dt) override;
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   static std::vector<std::shared_ptr<GameMechanism>> load(GameNode* parent, const GameDeserializeData& data);

   static void addObject(const std::shared_ptr<TmxObject>& object);
   static void addTilesVersion1();
   static void addTilesVersion2();
   static void merge();

   void reset();
   static void resetAll();

   const sf::Vector2f& getTilePosition() const;
   const sf::Vector2f& getPixelPosition() const;
   const sf::FloatRect& getPixelRect() const;

   void setEnabled(bool enabled) override;

protected:
   void collide();

   std::vector<Signal> _signal_plot;

   int32_t _tu = 0;
   int32_t _tv = 0;

   std::shared_ptr<sf::Texture> _texture;
   std::unique_ptr<sf::Sprite> _sprite;

   sf::Vector2f _tile_position;
   sf::Vector2f _position_px;
   sf::FloatRect _pixel_rect;

   std::optional<std::vector<sf::Vector2f>> _path;
   sf::Vector2f _move_offset_px;
   PathInterpolation<sf::Vector2f> _path_interpolation;

   bool _on = true;
   int32_t _tile_index = 0;
   float _tile_animation = 0.0f;
   int32_t _animation_offset = 0;
   uint32_t _signal_index = 0;
   uint32_t _time = 0u;
   int32_t _group_id = 0;  // only for debugging purposes

   Settings _settings;
};
