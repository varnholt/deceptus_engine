#pragma once

#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

#include <box2d/box2d.h>

#include <cstdint>

class GameNode;
struct TmxObject;

class Rope : public GameMechanism, public GameNode
{
public:
   Rope(GameNode* parent);
   std::string_view objectName() const override;

   void draw(sf::RenderTarget& color, sf::RenderTarget& normal) override;
   void update(const sf::Time& dt) override;
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   virtual void setup(const GameDeserializeData& data);

   sf::Vector2i getPixelPosition() const;
   void setPixelPosition(const sf::Vector2i& pixelPosition);

protected:
   int32_t _segment_count = 7;
   float _segment_length_m = 0.01f;

   std::vector<b2Body*> _chain_elements;
   std::shared_ptr<sf::Texture> _texture;

private:
   void pushChain(float impulse);

   sf::Vector2i _position_px;
   sf::FloatRect _bounding_box;

   // attachment of the 1st end of the rope
   b2BodyDef _anchor_a_def;
   b2Body* _anchor_a_body = nullptr;
   b2EdgeShape _anchor_a_shape;
   std::vector<b2Body*> _chain_bodies;

   // rope
   b2PolygonShape _rope_element_shape;
   b2FixtureDef _rope_element_fixture_def;
   b2RevoluteJointDef _joint_def;

   sf::IntRect _texture_rect_px;

   // wind
   bool _wind_enabled = true;
   float _push_time_s = 0.0f;
   float _push_interval_s = 5.0f;
   float _push_duration_s = 1.0f;
   float _push_strength = 0.02f;
   std::optional<float> _player_impulse;

   static int32_t _instance_counter;
};
