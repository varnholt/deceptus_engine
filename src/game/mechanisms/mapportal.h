#pragma once

#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

// sfml
#include "SFML/Graphics.hpp"

// box2d
#include <box2d/box2d.h>

// std
#include <filesystem>

struct TmxLayer;
struct TmxObject;
struct TmxTileSet;

class MapPortal : public GameMechanism, public GameNode
{
public:
   MapPortal(GameNode* parent = nullptr);

   void draw(sf::RenderTarget& window, sf::RenderTarget& normal) override;
   void update(const sf::Time& dt) override;
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   void setup(const GameDeserializeData& data);

   bool isPlayerAtMapPortal() const;
   void setPlayerAtMapPortal(bool isPlayerAtMapPortal);

   static void lock();
   static void unlock();
   static bool isLocked();

protected:
   sf::FloatRect _bounding_box;
   sf::Vector2u _tile_size;
   std::shared_ptr<sf::Texture> _texture;
   std::vector<sf::Sprite> _sprites;
   sf::Vector2f _tile_positions;
   int32_t _height = 0;
   bool _player_at_MapPortal = false;
   std::shared_ptr<MapPortal> _destination;

   static std::atomic<bool> _portal_lock;
};
