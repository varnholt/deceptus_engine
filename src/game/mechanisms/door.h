#pragma once

#include "constants.h"
#include "gamemechanism.h"
#include "gamenode.h"

#include <filesystem>

#include "SFML/Graphics.hpp"

#include "Box2D/Box2D.h"


struct TmxLayer;
struct TmxTileSet;

class Door : public GameMechanism, public GameNode
{
public:

   enum class Type
   {
      Bars,
      Conventional,
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

   void toggle();

   static std::vector<std::shared_ptr<GameMechanism>> load(
      TmxLayer *layer,
      TmxTileSet *tileSet,
      const std::filesystem::path& basePath,
      const std::shared_ptr<b2World>&
   );

   bool isPlayerAtDoor() const;
   void setPlayerAtDoor(bool isPlayerAtDoor);

   void reset();

   const sf::Vector2i& getTilePosition() const;


private:

   void setupBody(
      const std::shared_ptr<b2World>& world,
      float x_offset = 0.0f,
      float x_scale = 1.0f
   );

   void updateTransform();

   void updateBars(const sf::Time& dt);

   void open();
   void close();
   bool checkPlayerAtDoor() const;

   sf::Vector2u _tile_size;
   std::shared_ptr<sf::Texture> _texture;

   sf::VertexArray _door_quad{sf::Quads, 4};
   sf::Sprite _sprite_icon;

   Type _type = Type::Bars;

   State _initial_state = State::Closed;
   State _state = State::Closed;

   sf::Vector2i _tile_position;

   ItemType _required_item = ItemType::Invalid;

   float _offset = 0.0f;
   int32_t _height = 0;
   int32_t _tile_id = 0;
   bool _player_at_door = false;
   b2Body* _body = nullptr;
};

