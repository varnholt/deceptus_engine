#pragma once

#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

#include <SFML/Graphics.hpp>
#include <memory>

class Wind : public GameMechanism, public GameNode
{
public:
   explicit Wind(GameNode* parent = nullptr);
   ~Wind() override = default;
   std::string_view objectName() const override;

   void update(const sf::Time& dt) override;
   void draw(sf::RenderTarget& target, sf::RenderTarget& normal) override;
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   const sf::Vector2f& getDirection() const;

   static std::shared_ptr<Wind> deserialize(GameNode* parent, const GameDeserializeData& data);

private:
   sf::Vector2f _direction{0.f, 0.f};
   sf::FloatRect _area;
};
