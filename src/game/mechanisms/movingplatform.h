#pragma once

#include "framework/math/pathinterpolation.h"
#include "gamemechanism.h"
#include "gamenode.h"

#include "Box2D/Box2D.h"
#include <filesystem>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>


struct TmxLayer;
struct TmxObject;
struct TmxTileSet;


class MovingPlatform : public GameMechanism, public GameNode
{

public:
   MovingPlatform(GameNode* parent);

   static std::vector<std::shared_ptr<GameMechanism>> load(
      TmxLayer* layer,
      TmxTileSet* tileSet,
      const std::filesystem::path &basePath,
      const std::shared_ptr<b2World>& world
   );

   static void deserialize(TmxObject* tmx_object);
   static std::vector<std::shared_ptr<GameMechanism> > merge(const std::filesystem::path &base_path, const std::shared_ptr<b2World>& world);

   static void link(const std::vector<std::shared_ptr<GameMechanism>>& platforms, TmxObject* tmx_object);

   void draw(sf::RenderTarget& color, sf::RenderTarget& normal) override;
   void update(const sf::Time& dt) override;

   void setupBody(const std::shared_ptr<b2World>& world);
   void addSprite(const sf::Sprite&);
   b2Body* getBody();
   void setEnabled(bool enabled) override;

   const PathInterpolation& getInterpolation() const;
   const std::vector<sf::Vector2f>& getPixelPath() const;


private:

   void setupTransformDeprecated();
   void updateLeverLag(const sf::Time& dt);

   double CosineInterpolate(
      double y1,double y2,
      double mu
   );

   std::shared_ptr<sf::Texture> _texture_map;
   std::shared_ptr<sf::Texture> _normal_map;

   std::vector<sf::Sprite> _sprites;
   int32_t _animated_tile_index_0 = 0;
   int32_t _animated_tile_index_1 = 0;
   float _animation_elapsed = 0.0f;
   b2Body* _body = nullptr;
   sf::Vector2i _tile_positions;
   float _x = 0.0f;
   float _y = 0.0f;
   int32_t _element_count = 0;
   float _time = 0.0f;
   float _interpolation_value = 0.0f;
   bool _looped = false;
   bool _move_up = true;
   float _lever_lag = 0.0f;
   bool _initialized = false;
   PathInterpolation _interpolation;
   b2Vec2 _velocity;
   std::vector<sf::Vector2f> _pixel_path;
};

