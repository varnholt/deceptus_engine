#pragma once

#include <SFML/Graphics.hpp>

#include "Box2D/Box2D.h"

#include "gamedeserializedata.h"
#include "gamemechanism.h"
#include "gamenode.h"

struct TmxObject;


class SpikeBall : public GameMechanism, public GameNode
{
   public:

      struct SpikeConfig
      {
         // factor to control the push force when ball moves from right to left
         float _push_factor = 0.625f;

         // number of points retrieved from the given spline
         int32_t _spline_point_count = 25;

         // chain element setup
         int32_t _chain_element_count = 10;
         float _chain_element_distance = 0.3f;
         float _chain_element_width = 0.06f;
         float _chain_element_height = 0.0125f;
      };

      SpikeBall(GameNode* parent = nullptr);

      void draw(sf::RenderTarget& color, sf::RenderTarget& normal) override;
      void update(const sf::Time& dt) override;

      void setup(const GameDeserializeData& data);

      sf::Vector2i getPixelPosition() const;
      void setPixelPosition(const sf::Vector2i& pixelPosition);


   private:

      void drawChain(sf::RenderTarget& window);

      std::shared_ptr<sf::Texture> _texture;
      sf::Sprite _spike_sprite;
      sf::Sprite _box_sprite;
      sf::Sprite _chain_element_a;
      sf::Sprite _chain_element_b;

      sf::Vector2i _pixel_position;

      b2BodyDef _anchor_def;
      b2Body* _anchor_body = nullptr;
      b2EdgeShape _anchor_shape;

      b2RevoluteJointDef _joint_def;
      b2PolygonShape _chain_element_shape;
      b2FixtureDef _chain_element_fixture_def;
      std::vector<b2Body*> _chain_elements;

      b2Body* _ball_body = nullptr;
      b2CircleShape _ball_shape;
      b2BodyDef _ball_body_def;
      b2FixtureDef _ball_fixture_def;

      float _angle = 0.0f;
      SpikeConfig _config;
};

