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

/// \brief teleports the player to a linked destination portal when activated.
class Portal : public GameMechanism, public GameNode
{
public:
   /// \brief creates a portal mechanism instance.
   /// \param parent owning game node in the scene graph.
   Portal(GameNode* parent = nullptr);

   /// \brief returns the mechanism type name used by the serialization system.
   /// \return constant string view containing "Portal".
   std::string_view objectName() const override;

   /// \brief draws all portal tile sprites to the color target.
   /// \param window color render target.
   /// \param normal normal-map render target, unused by this mechanism.
   void draw(sf::RenderTarget& window, sf::RenderTarget& normal) override;

   /// \brief updates player interaction state and handles activation input.
   /// \param dt elapsed frame time, unused by this mechanism.
   void update(const sf::Time& dt) override;

   /// \brief returns the portal interaction bounds in pixel space.
   /// \return portal rectangle used for player overlap checks.
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   /// \brief loads portal instances from a tile layer and assembles stacked sprites.
   /// \param parent owning game node in the scene graph.
   /// \param data deserialization data for the current tmx layer.
   /// \return list of created portal mechanisms.
   static std::vector<std::shared_ptr<GameMechanism>> load(GameNode* parent, const GameDeserializeData& data);

   /// \brief links source and destination portals based on object polyline markers.
   /// \param portals portal mechanism list to resolve and connect.
   /// \param data deserialization data containing source and destination points.
   static void link(std::vector<std::shared_ptr<GameMechanism>>& portals, const GameDeserializeData& data);

   /// \brief appends one tile sprite to this portal's visual stack.
   /// \param sprite sprite slice belonging to this portal.
   void addSprite(const sf::Sprite&);

   /// \brief returns the linked destination portal.
   /// \return destination portal or nullptr when unlinked.
   std::shared_ptr<Portal> getDestination() const;

   /// \brief sets the linked destination portal.
   /// \param dst portal reached after activation.
   void setDestination(const std::shared_ptr<Portal>& dst);

   /// \brief returns the teleport exit position derived from the top sprite.
   /// \return pixel position used to place the player at the destination portal.
   sf::Vector2f getPortalPosition();

   /// \brief returns the portal base tile coordinate.
   /// \return tile-space position used during portal linking.
   const sf::Vector2f& getTilePosition() const;

   /// \brief acquires the global portal lock to prevent re-entrant teleports.
   static void lock();

   /// \brief releases the global portal lock after teleport completion.
   static void unlock();

   /// \brief reports whether portal transitions are currently locked globally.
   /// \return true if teleporting is locked.
   static bool isLocked();

protected:
   /// \brief starts the portal transition and teleports to the linked destination.
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
