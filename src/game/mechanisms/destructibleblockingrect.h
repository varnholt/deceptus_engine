#pragma once

#include "game/io/gamedeserializedata.h"
#include "game/level/fixturenode.h"
#include "game/mechanisms/gamemechanism.h"

#include <SFML/Graphics.hpp>
#include <memory>
#include <optional>
#include <string>

class DestructibleBlockingRect : public FixtureNode, public GameMechanism
{
public:
   DestructibleBlockingRect(GameNode* parent, const GameDeserializeData& data);

   std::string_view objectName() const override;
   void draw(sf::RenderTarget& color, sf::RenderTarget& normal) override;
   void update(const sf::Time& dt) override;
   std::optional<sf::FloatRect> getBoundingBoxPx() override;
   bool isDestructible() const override;

   void onHit(int32_t damage = 1);

private:
   enum class Alignment
   {
      Left,
      Right,
   };

   struct Config
   {
      int32_t frame_width{150};
      int32_t frame_height{163};
      int32_t frame_count{4};
      int32_t row{0};
      int32_t max_hits{4};
      std::string texture_path{"data/sprites/wooden_planks.png"};
      std::string hit_sound;
      std::string destroy_sound;
      int32_t z_index{0};
      Alignment alignment{Alignment::Left};
   };

   struct State
   {
      int32_t hits_left{0};
      int32_t current_frame{0};
      bool dead{false};
   };

   void setupBody(const GameDeserializeData& data);
   void setupSprite(const GameDeserializeData& data);
   void destroy();

   Config _config;
   State _state;

   std::shared_ptr<sf::Texture> _texture;
   std::shared_ptr<sf::Texture> _normal_map;
   std::unique_ptr<sf::Sprite> _sprite;
   sf::FloatRect _rect_px;

   b2Body* _body{nullptr};
   b2PolygonShape _shape;
};
