#pragma once

class GameNode;
struct TmxObject;

#include "game/io/gamedeserializedata.h"
#include "game/level/fixturenode.h"
#include "game/mechanisms/gamemechanism.h"

#include <box2d/box2d.h>

#include <filesystem>
#include <vector>

class CollapsingPlatform : public FixtureNode, public GameMechanism
{
public:
   struct Settings
   {
      float time_to_collapse_s = 1.0f;
      float destruction_speed = 30.0f;
      float fall_speed = 6.0f;
      float time_to_respawn_s = 4.0f;
      float fade_in_duration_s = 1.0f;
   };

   struct Block
   {
      float _x_px = 0.0f;
      float _y_px = 0.0f;
      float _shake_x_px = 0.0f;
      float _shake_y_px = 0.0f;
      float _fall_offset_y_px = 0.0f;
      float _elapsed_s = 0.0f;
      float _fall_speed = 0.0f;
      float _destruction_speed = 0.0f;
      int32_t _sprite_row = 0;
      int32_t _sprite_column = 0;
      std::unique_ptr<sf::Sprite> _sprite;
      uint8_t _alpha = 255;

      void reset()
      {
         _shake_x_px = 0.0f;
         _shake_y_px = 0.0f;
         _fall_offset_y_px = 0.0f;
         _elapsed_s = 0.0f;
         _sprite_column = 0;
      }
   };

   CollapsingPlatform(GameNode* parent, const GameDeserializeData& data);

   void preload() override;
   void draw(sf::RenderTarget& target, sf::RenderTarget& normal) override;
   void update(const sf::Time& dt) override;
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   void beginContact(b2Contact* /*contact*/, FixtureNode* other);
   void endContact(FixtureNode* other);

   void updateRespawnAnimation();

private:
   void updateBlockSprites();
   void updateBlockDestruction(const sf::Time& dt);
   void updateRespawn(const sf::Time& dt);
   void updateShakeBlocks();
   void collapse();

   Settings _settings;
   float _elapsed_s = 0.0f;
   float _collapse_elapsed_s = 0.0f;
   bool _collapsed = false;
   bool _respawning = false;
   bool _foot_sensor_contact = false;
   sf::Time _collapse_time;
   sf::Time _time_since_collapse;
   int32_t _width_tl = 0;
   float _width_m = 0.0f;
   float _height_m = 0.0f;
   std::vector<Block> _blocks;
   sf::Vector2f _position_px;
   sf::FloatRect _rect_px;
   bool _played_shake_sample = false;

   // sf
   std::shared_ptr<sf::Texture> _texture;

   // b2d
   b2Body* _body = nullptr;
   b2Fixture* _fixture = nullptr;
   b2Vec2 _position_m;
   b2PolygonShape _shape;
   void resetAllBlocks();
};
