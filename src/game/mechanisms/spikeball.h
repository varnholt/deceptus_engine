#pragma once

#include <SFML/Graphics.hpp>

#include <box2d/box2d.h>


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
      float _ball_radius = 0.45f;
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

   void preload() override;
   void draw(sf::RenderTarget& color, sf::RenderTarget& normal) override;
   void update(const sf::Time& dt) override;
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   void setup(const GameDeserializeData& data);

   sf::Vector2i getPixelPosition() const;
   void setPixelPosition(const sf::Vector2i& pixel_position);

private:
   void drawChain(sf::RenderTarget& window);

   std::shared_ptr<sf::Texture> _texture;
   sf::Sprite _spike_sprite;
   sf::Sprite _box_sprite;
   sf::Sprite _chain_element_a;
   sf::Sprite _chain_element_b;

   sf::Vector2i _pixel_position;
   sf::FloatRect _rect;

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
   float _last_ball_x_velocity = 0.0f;
   int32_t _swing_counter = 0;
   SpikeConfig _config;
   int32_t _instance_id{0};
};
