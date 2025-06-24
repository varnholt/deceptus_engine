#pragma once

#include "game/animation/animation.h"
#include "game/constants.h"
#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

#include <filesystem>

#include "SFML/Graphics.hpp"

#include <box2d/box2d.h>

struct TmxLayer;
struct TmxTileSet;

class Door : public GameMechanism, public GameNode
{
public:
   enum class Version
   {
      Version1,
      Version2,
   };

   enum class State
   {
      Open,
      Opening,
      Closing,
      Closed,
   };

   Door(GameNode* parent);
   virtual ~Door();

   void draw(sf::RenderTarget& color, sf::RenderTarget& normal) override;
   void update(const sf::Time& dt) override;
   void setEnabled(bool enabled) override;
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   void open();
   void close();
   void toggle() override;

   bool setup(const GameDeserializeData& data);

   bool isPlayerAtDoor() const;
   void setPlayerAtDoor(bool isPlayerAtDoor);

   const sf::Vector2i& getTilePosition() const;
   const sf::FloatRect& getPixelRect() const;

private:
   void setupBody(const std::shared_ptr<b2World>& world);

   void updateTransform();
   void updateBars(const sf::Time& dt);
   bool checkPlayerAtDoor() const;

   std::unique_ptr<sf::Sprite> _sprite;
   std::shared_ptr<sf::Texture> _texture;
   std::optional<std::string> _sample_open;
   std::optional<std::string> _sample_close;
   std::shared_ptr<Animation> _animation_open;
   std::shared_ptr<Animation> _animation_close;
   std::shared_ptr<Animation> _animation_key;
   sf::FloatRect _player_at_door_rect;

   Version _version = Version::Version2;
   State _state = State::Closed;

   // for 'version 1'
   sf::VertexArray _door_quad{sf::PrimitiveType::Triangles, 4};
   sf::Vector2i _tile_position_tl;
   sf::FloatRect _pixel_rect;
   float _bar_offset = 0.0f;

   std::optional<std::string> _required_item;

   bool _can_be_closed = false;
   bool _automatic_close = false;
   std::optional<std::chrono::high_resolution_clock::time_point> _last_toggle_time;

   bool _player_at_door = false;
   b2Body* _body = nullptr;
};
