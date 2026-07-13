#pragma once

#include "framework/tools/sfmlshader.h"
#include "game/io/gamedeserializedata.h"
#include "game/level/fixturenode.h"
#include "game/mechanisms/gamemechanism.h"

#include <SFML/Graphics.hpp>
#include <memory>
#include <optional>
#include <string>

/// \brief breakable obstacle that blocks movement until it takes enough damage.
class DestructibleBlockingRect : public FixtureNode, public GameMechanism
{
public:
   /// \brief creates a destructible blocker and initializes physics, sprite, audio, and shader state.
   /// \param parent parent node in the scene graph.
   /// \param data deserialize context with tmx properties and world pointers.
   DestructibleBlockingRect(GameNode* parent, const GameDeserializeData& data);

   /// \brief returns the mechanism type identifier.
   /// \return non-owning string view with value "DestructibleBlockingRect".
   std::string_view objectName() const override;

   /// \brief draws the blocker sprite with hit-flash shader effect.
   /// \param color color render target.
   /// \param normal normal render target.
   void draw(sf::RenderTarget& color, sf::RenderTarget& normal) override;

   /// \brief draws the blocker sprite with hit-flash shader effect and explicit render states (used in WASM to carry the level view).
   /// \param color color render target.
   /// \param normal normal render target.
   /// \param states render states to apply.
   void draw(sf::RenderTarget& color, sf::RenderTarget& normal, const sf::RenderStates& states) override;
   using GameMechanism::draw;

   /// \brief updates hit flash timing and death animation frame progression.
   /// \param dt elapsed frame time.
   void update(const sf::Time& dt) override;

   /// \brief returns collider bounds while alive.
   /// \return pixel bounding box, or std::nullopt once destroyed.
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   /// \brief returns melee/ranged hitboxes used to apply damage.
   /// \return list containing the blocker hitbox.
   const std::vector<Hitbox>& getHitboxes() override;

   /// \brief checks whether this mechanism supports the destructible interface.
   /// \return always true for this mechanism.
   bool isDestructible() const override;

   /// \brief applies damage, triggers hit feedback, and destroys the blocker when durability reaches zero.
   /// \param damage damage amount to subtract from remaining durability.
   void hit(int32_t damage = 1) override;

   /// \brief returns the static body used for collision.
   /// \return non-owning pointer to the blocker body.
   b2Body* getBody() const;

private:
   enum class Alignment
   {
      Left,
      Right,
   };

   /// \brief serialized configuration read from tmx custom properties.
   struct Config
   {
      int32_t frame_width{150};
      int32_t frame_height{163};
      int32_t frame_count{50};
      int32_t max_damage{30};
      int32_t row{0};
      std::string texture_path{"data/sprites/wooden_planks.png"};
      std::string hit_sound{"mechanism_destructible_blocking_rect_damage_1.wav"};
      std::string destroy_sound{"mechanism_destructible_blocking_rect_destroyed_1.wav"};
      int32_t z_index{0};
      Alignment alignment{Alignment::Left};
      float animation_speed{40.0f};
   };

   /// \brief mutable runtime state for durability and destruction animation.
   struct State
   {
      int32_t damage_left{0};
      float current_frame{0};
      bool dead{false};
   };

   /// \brief creates the static collision body, chunk coverage, and hitbox.
   /// \param data deserialize context with object geometry and physics world.
   void setupBody(const GameDeserializeData& data);

   /// \brief loads and positions the sprite and initial animation row.
   /// \param data deserialize context with object transform and resources.
   void setupSprite(const GameDeserializeData& data);

   /// \brief enters destroyed state, plays destroy audio, and disables collisions.
   void destroy();

   Config _config;
   State _state;

   std::shared_ptr<sf::Texture> _texture;
   std::shared_ptr<sf::Texture> _normal_map;
   std::unique_ptr<sf::Sprite> _sprite;
   sf::FloatRect _rect_px;

   b2Body* _body{nullptr};
   b2PolygonShape _shape;

   using HighResTimePoint = std::chrono::high_resolution_clock::time_point;
   std::optional<HighResTimePoint> _hit_time;
   sfcompat::Shader _flash_shader;
   float _hit_flash{0.0f};
   std::vector<Hitbox> _hitboxes;
};
