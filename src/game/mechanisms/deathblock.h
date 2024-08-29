#pragma once

#include "framework/math/pathinterpolation.h"
#include "game/constants.h"
#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

#include <box2d/box2d.h>

#include <array>
#include <filesystem>

struct TmxLayer;
struct TmxObject;
struct TmxTileSet;

class DeathBlock : public GameMechanism, public GameNode
{
public:

   DeathBlock(GameNode* parent = nullptr);
   void draw(sf::RenderTarget& color, sf::RenderTarget& normal) override;
   void update(const sf::Time& dt) override;
   std::optional<sf::FloatRect> getBoundingBoxPx() override;
   void setup(const GameDeserializeData& data);

private:
   enum class Mode
   {
      Invalid,
      AlwaysOn,
      OnContact,
      Interval,
      Rotate
   };

   struct Spike
   {
      enum class State
      {
         Extracting,
         Extracted,
         Retracting,
         Retracted,
      };

      enum Orientation
      {
         Up = 0,
         Down = 1,
         Left = 2,
         Right = 3,
      };

      Spike()
      {
         _collision_rect_absolute.width = PIXELS_PER_TILE;
         _collision_rect_absolute.height = PIXELS_PER_TILE;
      }

      bool hasChanged() const
      {
         return _sprite_index != _sprite_index_prev;
      }

      void updateIndex()
      {
         _sprite_index_prev = _sprite_index;
         _sprite_index = static_cast<int32_t>(_state_time_s);
      }

      void extract(const sf::Time& dt);
      void extracted(const sf::Time& dt, const sf::Time& time_on);
      void retract(const sf::Time& dt);
      void retracted(const sf::Time& dt, const sf::Time& time_off);

      State _state{State::Retracted};
      sf::Time _wait_time;
      sf::Time _wait_offset;
      float _state_time_s{0.0f};
      sf::Sprite _sprite;
      int32_t _sprite_index{0};
      int32_t _sprite_index_prev{-1};
      sf::IntRect _collision_rect_relative;
      sf::IntRect _collision_rect_absolute;
      bool _active{true};
   };

   void setupTransform();
   void setupBody(const std::shared_ptr<b2World>& world);
   void updateLeverLag(const sf::Time& dt);
   void updateCollision();
   void updateStates(const sf::Time& dt);
   void updateBoundingBox();
   void updateSprites();
   void updatePosition(const sf::Time& dt);
   void updateStatesInterval(const sf::Time& dt);

   //     +---+
   //     | 0 |
   // +---+---+---+
   // | 3 |   | 1 |
   // +---+---+---+
   //     | 2 |
   //     +---+

   sf::Vector2f _pixel_positions;
   sf::FloatRect _rect;
   b2Body* _body = nullptr;
   std::vector<b2Vec2> _path;
   PathInterpolation<b2Vec2> _interpolation;
   float _lever_lag = 1.0f;  // maybe make them switchable as well?
   float _velocity = 1.0f;
   sf::Time _time_on;
   sf::Time _time_off;
   std::array<Spike, 4> _spikes;
   sf::Sprite _center_sprite;
   float _center_sprite_time_s{0.0f};
   int32_t _center_sprite_index{0};
   Mode _mode{Mode::Interval};
   std::shared_ptr<sf::Texture> _texture;
   int32_t _damage{100};
};
