#pragma once

#include <box2d/box2d.h>
#include <SFML/Graphics.hpp>

#include <memory>
#include <optional>
#include <vector>

#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

class Fan : public GameMechanism, public GameNode
{
public:
   Fan(GameNode* parent = nullptr);
   std::string_view objectName() const override;

   void draw(sf::RenderTarget& color, sf::RenderTarget& normal) override;
   void update(const sf::Time& dt) override;
   std::optional<sf::FloatRect> getBoundingBoxPx() override;
   void setEnabled(bool enabled) override;

   const sf::FloatRect& getPixelRect() const;

   static std::shared_ptr<Fan> deserialize(GameNode* parent, const GameDeserializeData& data);

private:
   struct FanSection
   {
      sf::Vector2i tile_position_px;
      sf::Vector2f direction;
      sf::FloatRect rect;
      b2Body* body = nullptr;
      std::unique_ptr<sf::Sprite> sprite;
      float scroll_offset = 0.0f;

      FanSection(const std::shared_ptr<sf::Texture>& tex)
      {
         sprite = std::make_unique<sf::Sprite>(*tex);
         sprite->setTexture(*tex);
      }
   };

   void updateSprite();
   void collide();

   static void placeTile(const std::shared_ptr<Fan>& fan, const GameDeserializeData& data, int i, int j);

   std::vector<FanSection> _tiles;
   sf::Vector2f _direction;
   std::string _direction_string;
   sf::FloatRect _pixel_rect;
   float _speed = 1.0f;
   float _lever_lag = 1.0f;

   std::shared_ptr<sf::Texture> _texture;
};
