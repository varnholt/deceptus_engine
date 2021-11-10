#pragma once

#include <filesystem>

#include "constants.h"
#include "gamemechanism.h"
#include "fixturenode.h"
#include "Box2D/Box2D.h"
#include "SFML/Graphics.hpp"

struct TmxObject;

class ConveyorBelt : public FixtureNode, public GameMechanism
{

   public:

      ConveyorBelt(
         GameNode* parent,
         const std::shared_ptr<b2World>& world,
         TmxObject* tmxObject,
         const std::filesystem::path& basePath
      );

      b2Body *getBody() const;
      float getVelocity() const;
      void setVelocity(float velocity);

      void draw(sf::RenderTarget& color, sf::RenderTarget& normal) override;
      void update(const sf::Time& dt) override;
      void setEnabled(bool enabled) override;

      sf::IntRect getPixelRect() const;


      static void resetBeltState();
      static void processContact(b2Contact *contact);
      static void processFixtureNode(FixtureNode* fixtureNode, b2Body* collidingBody);


      void updateSprite();

   private:

      b2Body* _body = nullptr;
      b2Vec2 _position_b2d;
      sf::Vector2f _position_sfml;
      b2PolygonShape _shape;
      sf::IntRect _belt_pixel_rect;
      sf::IntRect _arrow_pixel_rect;
      float _elapsed = 0.0f;
      bool _points_right = true;
      float _lever_lag = 1.0f;

      std::shared_ptr<sf::Texture> _texture;
      std::vector<sf::Sprite> _belt_sprites;
      std::vector<sf::Sprite> _arrow_sprites;

      // bool mActive = true;
      float _velocity = -0.2f;

      static std::vector<b2Body*> __bodies_on_belt;
};

