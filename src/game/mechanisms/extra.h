#pragma once

#include <functional>
#include <memory>
#include <vector>

#include <SFML/Graphics.hpp>

#include "game/animation/animationpool.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

struct ExtraItem;
class GameNode;
struct GameDeserializeData;
struct InventoryItem;
class TileMap;
struct TmxLayer;
struct TmxTileSet;

/// \brief represents a collectible extra that can be animated, gated, and added to inventory.
class Extra : public GameMechanism, public GameNode
{
public:
   /// \brief creates an extra mechanism.
   /// \param parent parent node in the scene graph.
   Extra(GameNode* parent = nullptr);

   /// \brief returns the mechanism registry name.
   /// \return string view containing `Extra`.
   std::string_view objectName() const override;

   /// \brief loads visuals, interaction flags, audio, and animations from TMX properties.
   /// \param data deserialize context containing TMX object properties.
   /// \return true when TMX object data exists and setup completes.
   bool deserialize(const GameDeserializeData& data);

   /// \brief draws spawn and pickup animations, then draws the item when active and visible.
   /// \param target render target.
   /// \param normal normal-map render target (unused).
   void draw(sf::RenderTarget& target, sf::RenderTarget& /*normal*/) override;

   /// \brief updates animations and handles pickup checks against the player rectangle.
   /// \param dt elapsed frame time.
   void update(const sf::Time& dt) override;

   /// \brief returns the collectible interaction rectangle.
   /// \return extra rectangle in pixels.
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   /// \brief marks the extra as spawned, applies a position offset, and starts spawn animation playback when configured.
   /// \param offset pixel offset applied to the pickup rect and all animations before becoming collectable.
   void spawn(sf::Vector2f offset = {});

   /// \brief writes the active flag into the save-state json so pickup survives level reloads.
   /// \param json json object that receives the entry keyed by this extra's name.
   void serializeState(nlohmann::json& json) override;

   /// \brief restores the active flag from a previously serialized save-state entry.
   /// \param json json object containing the saved active field.
   void deserializeState(const nlohmann::json& json) override;

   using ExtraCallback = std::function<void(const std::string&)>;

   bool _active{true};
   bool _spawn_required{false};
   bool _spawned{false};
   std::string _name;
   std::optional<std::string> _sample;
   std::unique_ptr<sf::Sprite> _sprite;
   std::shared_ptr<sf::Texture> _texture;
   sf::FloatRect _rect;
   std::vector<ExtraCallback> _callbacks;
   bool _requires_button_press{false};
   bool _is_treasure{false};  //!< when true, pickup is routed to treasures instead of inventory

   std::vector<std::shared_ptr<Animation>> _animations_main;
   std::shared_ptr<Animation> _animation_spawn;
   std::shared_ptr<Animation> _animation_pickup;
   std::vector<std::shared_ptr<Animation>>::iterator _animations_main_it;
};
