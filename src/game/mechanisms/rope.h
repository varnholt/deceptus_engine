#pragma once

#include "gamemechanism.h"
#include "gamenode.h"

#include <Box2D/Box2D.h>

#include <cstdint>


class GameNode;
struct TmxObject;

class Rope : public GameMechanism, public GameNode
{
   public:

      Rope(GameNode* parent);

      void draw(sf::RenderTarget& color, sf::RenderTarget& normal) override;
      void update(const sf::Time& dt) override;

      virtual void setup(TmxObject* tmxObject, const std::shared_ptr<b2World>& world);

      sf::Vector2i getPixelPosition() const;
      void setPixelPosition(const sf::Vector2i& pixelPosition);


   protected:

      int32_t _segment_count = 7;
      float _segment_length_m = 0.01f;

      std::vector<b2Body*> _chain_elements;


   private:

      sf::Vector2i _position_px;

      // attachment of the 1st end of the rope
      b2BodyDef _anchor_a_def;
      b2Body* _anchor_a_body = nullptr;
      b2EdgeShape _anchor_a_shape;

      // rope
      b2PolygonShape _rope_element_shape;
      b2FixtureDef _rope_element_fixture_def;
      b2RevoluteJointDef _joint_def;

      std::shared_ptr<sf::Texture> _texture;
      sf::IntRect _texture_rect_px;
};
