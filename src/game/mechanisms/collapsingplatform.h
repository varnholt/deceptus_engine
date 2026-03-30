#pragma once

class GameNode;
struct TmxObject;

#include "game/io/gamedeserializedata.h"
#include "game/level/fixturenode.h"
#include "game/mechanisms/gamemechanism.h"

#include "box2d/box2d.h"

#include <filesystem>
#include <vector>

/// \brief platform that shakes, collapses under the player, and later respawns with a fade-in.
class CollapsingPlatform : public FixtureNode, public GameMechanism
{
public:
   /// \brief tunable timings and speeds for collapse, destruction animation, and respawn.
   struct Settings
   {
      float time_to_collapse_s = 1.0f;
      float destruction_speed = 30.0f;
      float fall_speed = 6.0f;
      float time_to_respawn_s = 4.0f;
      float fade_in_duration_s = 1.0f;
   };

   /// \brief runtime state for one visual tile segment of the platform.
   struct Block
   {
      float _x_px = 0.0f;
      float _y_px = 0.0f;
      float _shake_x_px = 0.0f;
      float _shake_y_px = 0.0f;
      float _fall_offset_y_px = 0.0f;
      float _elapsed_s = 0.0f;
      float _fall_speed = 0.0f;
      float _destruction_speed = 0.0f;
      int32_t _sprite_row = 0;
      int32_t _sprite_column = 0;
      std::unique_ptr<sf::Sprite> _sprite;
      uint8_t _alpha = 255;

      /// \brief resets transient animation offsets and frame state to defaults.
      void reset()
      {
         _shake_x_px = 0.0f;
         _shake_y_px = 0.0f;
         _fall_offset_y_px = 0.0f;
         _elapsed_s = 0.0f;
         _sprite_column = 0;
      }
   };

   /// \brief builds platform blocks, collision shape, and body from tmx properties.
   /// \param parent owning game node in the scene graph.
   /// \param data deserialize context with tmx object data and world pointers.
   CollapsingPlatform(GameNode* parent, const GameDeserializeData& data);

   /// \brief returns the mechanism type identifier.
   /// \return non-owning string view with value "CollapsingPlatform".
   std::string_view objectName() const override;

   /// \brief preloads the crumble sound used while the platform is shaking.
   void preload() override;

   /// \brief draws all visible platform blocks.
   /// \param target render target.
   /// \param normal normal render target.
   void draw(sf::RenderTarget& target, sf::RenderTarget& normal) override;

   /// \brief updates collapse timing, block animation, and respawn behavior.
   /// \param dt elapsed frame time.
   void update(const sf::Time& dt) override;

   /// \brief returns the platform bounds in pixel coordinates.
   /// \return rectangle used for culling and respawn overlap checks.
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   /// \brief marks foot contact when the player foot sensor touches the platform.
   /// \param contact box2d contact object.
   /// \param other other fixture node involved in the contact.
   void beginContact(b2Contact* /*contact*/, FixtureNode* other);

   /// \brief clears foot contact when the player foot sensor leaves the platform.
   /// \param other other fixture node that ended contact.
   void endContact(FixtureNode* other);

   /// \brief updates block alpha during the fade-in phase after respawn starts.
   void updateRespawnAnimation();

private:
   /// \brief updates sprite positions and texture rectangles for all blocks.
   void updateBlockSprites();

   /// \brief advances collapse destruction frames and falling offsets while collapsed.
   /// \param dt elapsed frame time.
   void updateBlockDestruction(const sf::Time& dt);

   /// \brief advances collapse timer and starts respawn when allowed.
   /// \param dt elapsed frame time.
   void updateRespawn(const sf::Time& dt);

   /// \brief applies shake offsets to blocks while collapse countdown is active.
   void updateShakeBlocks();

   /// \brief enters collapsed state and disables the physics body.
   void collapse();

   Settings _settings;
   float _elapsed_s = 0.0f;
   float _collapse_elapsed_s = 0.0f;
   bool _collapsed = false;
   bool _respawning = false;
   bool _foot_sensor_contact = false;
   sf::Time _collapse_time;
   sf::Time _time_since_collapse;
   int32_t _width_tl = 0;
   float _width_m = 0.0f;
   float _height_m = 0.0f;
   std::vector<Block> _blocks;
   sf::Vector2f _position_px;
   sf::FloatRect _rect_px;
   bool _played_shake_sample = false;

   // sf
   std::shared_ptr<sf::Texture> _texture;

   // b2d
   b2Body* _body = nullptr;
   b2Fixture* _fixture = nullptr;
   b2Vec2 _position_m;
   b2PolygonShape _shape;

   /// \brief resets every block to its initial animation state.
   void resetAllBlocks();
};
