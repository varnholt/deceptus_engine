#pragma once

class GameNode;
struct TmxObject;

#include "gamedeserializedata.h"
#include "fixturenode.h"
#include "gamemechanism.h"

#include "Box2D/Box2D.h"
#include <filesystem>
#include <vector>


class CollapsingPlatform : public FixtureNode, public GameMechanism
{
public:

   struct Block
   {
      int32_t _sprite_index = 0;
   };

   CollapsingPlatform(
      GameNode* parent,
      const GameDeserializeData& data
   );

   void draw(sf::RenderTarget& target, sf::RenderTarget& normal) override;
   void update(const sf::Time& dt) override;

   void beginContact();
   void endContact();

private:
   float _animation_offset_s = 0.0f;
   float _elapsed_s = 0.0f;
   float _collapse_elapsed_s = 0.0f;
   bool _collapsed = false;
   int32_t _contact_count = 0;
   sf::Time _collapse_time;
   int32_t _width_tl = 0;
   float _width_m = 0.0f;
   float _height_m = 0.0f;
   std::vector<Block> _blocks;
   sf::Vector2f _position_px;

   // sf
   std::shared_ptr<sf::Texture> _texture;
   std::vector<sf::Sprite> _sprites;

   // b2d
   b2Body* _body = nullptr;
   b2Fixture* _fixture = nullptr;
   b2Vec2 _position_m;
   b2ChainShape _shape;
};

