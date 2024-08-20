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
   enum SpikeOrientation
   {
      Up = 0,
      Right = 1,
      Down = 2,
      Left = 3,
      Center = 4,
   };

   enum class Mode
   {
      Invalid,
      AlwaysOn,
      OnContact,
      Interval,
   };

   enum class State
   {
      Extracting,
      Extracted,
      Retracting,
      Retracted
   };

   void setupTransform();
   void setupBody(const std::shared_ptr<b2World>& world);
   void updateLeverLag(const sf::Time& dt);
   void updateCollision();
   void updateStates(const sf::Time& dt);
   void updateBoundingBox();
   void updateSprites();
   void updatePosition(const sf::Time& dt);

   //     +---+
   //     | 0 |
   // +---+---+---+
   // | 3 | 4 | 1 |
   // +---+---+---+
   //     | 2 |
   //     +---+

   std::array<sf::Sprite, 5> _sprites;

   std::array<int32_t, 5> _sprite_indices = {0, 0, 0, 0, 0};
   std::array<float, 5> _state_times = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};

   // std::array<sf::Vector2i, 5> _offsets =
   //    {sf::Vector2i{1, 0}, sf::Vector2i{0, 1}, sf::Vector2i{1, 1}, sf::Vector2i{1, 2}, sf::Vector2i{2, 1}};

   std::array<sf::IntRect, 4> _collision_rects = {
      sf::IntRect{1 * PIXELS_PER_TILE, 0 * PIXELS_PER_TILE, PIXELS_PER_TILE, PIXELS_PER_TILE},
      sf::IntRect{0 * PIXELS_PER_TILE, 1 * PIXELS_PER_TILE, PIXELS_PER_TILE, PIXELS_PER_TILE},
      sf::IntRect{1 * PIXELS_PER_TILE, 2 * PIXELS_PER_TILE, PIXELS_PER_TILE, PIXELS_PER_TILE},
      sf::IntRect{2 * PIXELS_PER_TILE, 1 * PIXELS_PER_TILE, PIXELS_PER_TILE, PIXELS_PER_TILE}
   };

   sf::Vector2f _pixel_positions;
   sf::FloatRect _rect;
   std::vector<sf::Vector2f> _pixel_paths;

   b2Body* _body = nullptr;
   std::vector<b2Vec2> _path;
   PathInterpolation<b2Vec2> _interpolation;
   float _lever_lag = 1.0f;  // maybe make them switchable as well?
   float _velocity = 1.0f;

   sf::Time _time_on;
   sf::Time _time_off;
   std::array<State, 4> _states{State::Extracted, State::Extracted, State::Extracted, State::Extracted};
   std::array<sf::Time, 4> _times;

   Mode _mode{Mode::Interval};

   std::shared_ptr<sf::Texture> _texture;
};
