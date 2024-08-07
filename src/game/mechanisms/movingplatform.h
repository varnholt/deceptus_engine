#pragma once

#include "framework/math/pathinterpolation.h"
#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

#include <box2d/box2d.h>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <filesystem>

struct TmxLayer;
struct TmxObject;
struct TmxTileSet;

class MovingPlatform : public GameMechanism, public GameNode
{
public:
   MovingPlatform(GameNode* parent);

   static std::vector<std::shared_ptr<GameMechanism>> load(GameNode* parent, const GameDeserializeData& data);
   static void deserialize(const std::shared_ptr<TmxObject>& tmx_object);
   static std::vector<std::shared_ptr<GameMechanism>> merge(GameNode* parent, const GameDeserializeData& data);

   static void link(const std::vector<std::shared_ptr<GameMechanism>>& platforms, const GameDeserializeData& data);

   void draw(sf::RenderTarget& color, sf::RenderTarget& normal) override;
   void update(const sf::Time& dt) override;
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   void setupBody(const std::shared_ptr<b2World>& world);
   void addSprite(const sf::Sprite&);
   b2Body* getBody();
   void setEnabled(bool enabled) override;

   const std::vector<sf::Vector2f>& getPixelPath() const;
   float getDx() const;

private:
   void setupTransform();
   void updateLeverLag(const sf::Time& dt);

   double cosineInterpolate(double y1, double y2, double mu);

   std::shared_ptr<sf::Texture> _texture_map;
   std::shared_ptr<sf::Texture> _normal_map;

   std::vector<sf::Sprite> _sprites;
   int32_t _animated_tile_index_0 = 0;
   int32_t _animated_tile_index_1 = 0;
   float _animation_elapsed = 0.0f;
   b2Body* _body = nullptr;
   sf::Vector2i _tile_positions;
   int32_t _element_count = 0;
   float _lever_lag = 0.0f;
   bool _initialized = false;
   PathInterpolation<b2Vec2> _interpolation;
   b2Vec2 _velocity;
   std::vector<sf::Vector2f> _pixel_path;
   sf::FloatRect _rect;
   sf::Vector2f _pos;
   sf::Vector2f _pos_prev;
};
