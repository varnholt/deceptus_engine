#pragma once

#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

// sfml
#include "SFML/Graphics.hpp"

// box2d
#include "box2d/box2d.h"

// std
#include <filesystem>

struct TmxLayer;
struct TmxObject;
struct TmxTileSet;

class Portal : public GameMechanism, public GameNode
{
public:
   Portal(GameNode* parent = nullptr);
   std::string_view objectName() const override;

   void draw(sf::RenderTarget& window, sf::RenderTarget& normal) override;
   void update(const sf::Time& dt) override;
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   static std::vector<std::shared_ptr<GameMechanism>> load(GameNode* parent, const GameDeserializeData& data);

   static void link(std::vector<std::shared_ptr<GameMechanism>>& portals, const GameDeserializeData& data);

   void addSprite(const sf::Sprite&);

   std::shared_ptr<Portal> getDestination() const;
   void setDestination(const std::shared_ptr<Portal>& dst);

   sf::Vector2f getPortalPosition();
   const sf::Vector2f& getTilePosition() const;

   static void lock();
   static void unlock();
   static bool isLocked();

protected:
   void use();

   sf::Clock _portal_clock;
   sf::FloatRect _rect;
   sf::Vector2u _tile_size;
   std::shared_ptr<sf::Texture> _texture;
   std::vector<sf::Sprite> _sprites;
   sf::Vector2f _tile_positions;
   int32_t _height = 0;
   bool _player_at_portal = false;
   std::shared_ptr<Portal> _destination;
   bool _player_intersects{false};

   static std::atomic<bool> _portal_lock;
};
