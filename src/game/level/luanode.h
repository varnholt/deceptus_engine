#pragma once

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <variant>

// box2d
#include "box2d/box2d.h"

// sfml
#include "SFML/Graphics.hpp"

// game
#include "game/animation/detonationanimation.h"
#include "game/level/enemydescription.h"
#include "game/level/gamenode.h"
#include "game/level/hitbox.h"
#include "game/mechanisms/gamemechanism.h"
#include "game/weapons/weapon.h"

struct lua_State;

/// \brief scripted enemy node that bridges Lua behavior, rendering, physics, audio, and weapons.
struct LuaNode : public GameMechanism, public GameNode
{
   using HighResTimePoint = std::chrono::high_resolution_clock::time_point;

   /// \brief creates a scripted node and allocates its box2d body.
   /// \param parent parent node in the scene graph.
   /// \param filename lua script file path used by this node.
   LuaNode(GameNode* parent, const std::string& filename);

   /// \brief stops script execution and releases runtime resources.
   ~LuaNode();

   /// \brief returns the static runtime type name used by mechanism systems.
   /// \return non-owning name of this mechanism type.
   std::string_view objectName() const override;

   /// \brief draws enemy sprites and optional debug overlays.
   /// \param window color render target.
   /// \param normal normal-map render target.
   void draw(sf::RenderTarget& window, sf::RenderTarget& normal) override;
   void draw(sf::RenderTarget& window, sf::RenderTarget& normal, const sf::RenderStates& states) override;
   using GameMechanism::draw;

   /// \brief returns cached world-space bounding box built from active hitboxes.
   /// \return bounding rectangle in pixels, or std::nullopt when no hitboxes exist.
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   /// \brief reports whether this enemy can be damaged by the player.
   /// \return true because LuaNode enemies are destructible.
   bool isDestructible() const override;

   /// \brief exposes current hitbox list used for player attacks.
   /// \return reference to the internal hitbox vector.
   const std::vector<Hitbox>& getHitboxes() override;

   /// \brief initializes script state, body fixtures, and shaders.
   void initialize();

   /// \brief applies parsed enemy description data to runtime fields.
   void deserializeEnemyDescription();

   /// \brief creates lua state, registers callbacks, and executes the script file.
   void setupLua();

   /// \brief loads sprite texture from script properties and creates the first sprite.
   void setupTexture();

   /// \brief syncs rendered position from box2d body coordinates.
   void updatePosition();

   /// \brief applies script-facing velocity constraints and friction behavior.
   void updateVelocity();

   /// \brief updates weapon instances attached to this node.
   /// \param dt elapsed frame time.
   void updateWeapons(const sf::Time& dt);

   /// \brief refreshes hitbox world positions based on current node position.
   void updateHitboxOffsets();

   /// \brief adds a circle fixture shape to the pending body setup.
   /// \param radius circle radius in box2d world units.
   /// \param x circle center x in body-local world units.
   /// \param y circle center y in body-local world units.
   void addShapeCircle(float radius, float x, float y);

   /// \brief adds a polygon fixture shape to the pending body setup.
   /// \param points polygon vertex array in body-local world units.
   /// \param size number of vertices in \p points.
   void addShapePoly(const b2Vec2* points, int32_t size);

   /// \brief adds an axis-aligned rectangle fixture shape to the pending body setup.
   /// \param width half-width in box2d world units.
   /// \param height half-height in box2d world units.
   /// \param center_x center x in body-local world units.
   /// \param center_y center y in body-local world units.
   void addShapeRect(float width, float height, float center_x, float center_y);

   /// \brief adds an eight-vertex beveled rectangle fixture shape.
   /// \param width total width in box2d world units.
   /// \param height total height in box2d world units.
   /// \param bevel corner inset size in box2d world units.
   /// \param offset_x local x offset in box2d world units.
   /// \param offset_y local y offset in box2d world units.
   void addShapeRectBevel(float width, float height, float bevel, float offset_x = 0.0f, float offset_y = 0.0f);

   /// \brief attaches a prepared weapon instance to this node.
   /// \param weapon weapon object to append.
   void addWeapon(const std::shared_ptr<Weapon>& weapon);

   /// \brief triggers world boom feedback at a position.
   /// \param x center x in pixels.
   /// \param y center y in pixels.
   /// \param intensity boom strength used by the camera effect.
   void boom(float x, float y, float intensity);

   /// \brief spawns the predefined large detonation visual effect.
   /// \param x center x in pixels.
   /// \param y center y in pixels.
   void playDetonationAnimationHuge(float x, float y);

   /// \brief spawns a detonation effect from script-defined ring parameters.
   /// \param rings ring descriptions used to generate particle bursts.
   void playDetonationAnimation(const std::vector<DetonationAnimation::DetonationRing>& rings);

   /// \brief applies immediate damage and force direction to the player.
   /// \param damage damage amount.
   /// \param forceX force direction x.
   /// \param forceY force direction y.
   void damagePlayer(int32_t damage, float forceX, float forceY);

   /// \brief applies damage when the player is within a radius around a point.
   /// \param damage damage amount.
   /// \param x center x in pixels.
   /// \param y center y in pixels.
   /// \param radius radius in pixels.
   void damagePlayerInRadius(int32_t damage, float x, float y, float radius);

   /// \brief reads current box2d body linear velocity.
   /// \return velocity vector in box2d world units.
   b2Vec2 getLinearVelocity() const;

   /// \brief sets box2d body linear velocity.
   /// \param vel velocity vector in box2d world units.
   void setLinearVelocity(const b2Vec2& vel);

   /// \brief applies impulse to the center of the box2d body.
   /// \param vel impulse vector in box2d world units.
   void applyLinearImpulse(const b2Vec2& vel);

   /// \brief applies force to the center of the box2d body.
   /// \param force force vector in box2d world units.
   void applyForce(const b2Vec2& force);

   /// \brief fires a weapon by index using origin and direction vectors.
   /// \param index weapon index inside the internal weapon list.
   /// \param from origin in box2d world units.
   /// \param to direction or target vector expected by the weapon implementation.
   void useWeapon(size_t index, b2Vec2 from, b2Vec2 to);

   /// \brief grants a skill flag to the player save state.
   /// \param skill_type skill bitmask to add.
   void addPlayerSkill(int32_t);

   /// \brief removes a skill flag from the player save state.
   /// \param skill_type skill bitmask to remove.
   void removePlayerSkill(int32_t);

   /// \brief changes the box2d body type to dynamic.
   void makeDynamic();

   /// \brief changes the box2d body type to static.
   void makeStatic();

   /// \brief counts fixtures overlapping an axis-aligned box query.
   /// \param aabb query bounds in box2d world units.
   /// \return number of intersecting fixtures.
   int32_t queryAABB(const b2AABB& aabb);

   /// \brief counts fixtures hit by a ray cast.
   /// \param point1 ray start in box2d world units.
   /// \param point2 ray end in box2d world units.
   /// \return number of hit fixtures.
   int32_t queryRaycast(const b2Vec2& point1, const b2Vec2& point2);

   /// \brief enables or disables the box2d body simulation.
   /// \param active true to enable simulation, false to disable.
   void setActive(bool active);

   /// \brief sets fixture damage metadata used on player collision.
   /// \param damage damage amount.
   void setDamageToPlayer(int32_t damage);

   /// \brief sets gravity scale multiplier on the body.
   /// \param scale gravity scale factor.
   void setGravityScale(float scale);

   /// \brief sets body transform directly.
   /// \param position body position in box2d world units.
   /// \param angle body rotation in radians.
   void setTransform(const b2Vec2& position, float angle = 0.0);

   /// \brief appends a sprite using the node texture.
   void addSprite();

   /// \brief sets origin of a sprite layer.
   /// \param id sprite layer index.
   /// \param x origin x in pixels.
   /// \param y origin y in pixels.
   void setSpriteOrigin(int32_t id, float x, float y);

   /// \brief sets local position offset of a sprite layer.
   /// \param id sprite layer index.
   /// \param x offset x in pixels.
   /// \param y offset y in pixels.
   void setSpriteOffset(int32_t id, float x, float y);

   /// \brief updates texture rectangle of a sprite layer.
   /// \param id sprite layer index.
   /// \param x texture left in pixels.
   /// \param y texture top in pixels.
   /// \param w texture width in pixels.
   /// \param h texture height in pixels.
   void updateSpriteRect(int32_t id, int32_t x, int32_t y, int32_t w, int32_t h);

   /// \brief sets scale factors of a sprite layer.
   /// \param id sprite layer index.
   /// \param x_scale horizontal scale factor.
   /// \param y_scale vertical scale factor.
   void setSpriteScale(int32_t id, float x_scale, float y_scale);

   /// \brief sets rgba tint of a sprite layer.
   /// \param id sprite layer index.
   /// \param r red channel.
   /// \param g green channel.
   /// \param b blue channel.
   /// \param a alpha channel.
   void setSpriteColor(int32_t id, uint8_t r, uint8_t g, uint8_t b, uint8_t a);

   /// \brief toggles visibility of one sprite layer.
   /// \param id sprite layer index.
   /// \param visible true to draw the sprite layer.
   void setSpriteVisible(int32_t id, bool visible);

   /// \brief toggles visibility of the whole node.
   /// \param visible true to draw the node.
   void setVisible(bool visible);

   /// \brief stores current key bitmask for script-side polling.
   /// \param keys pressed key flags combined from KeyPressed.
   void setKeysPressed(int32_t keys);

   /// \brief updates one debug rectangle by index.
   /// \param index debug rectangle index.
   /// \param left_px rectangle left in pixels.
   /// \param top_px rectangle top in pixels.
   /// \param width_px rectangle width in pixels.
   /// \param height_px rectangle height in pixels.
   void updateDebugRect(int32_t index, float left_px, float top_px, float width_px, float height_px);

   /// \brief appends one empty debug rectangle.
   void addDebugRect();

   /// \brief appends one hitbox in node-local pixel coordinates.
   /// \param left_px rectangle left in pixels.
   /// \param top_px rectangle top in pixels.
   /// \param width_px rectangle width in pixels.
   /// \param height_px rectangle height in pixels.
   void addHitbox(int32_t left_px, int32_t top_px, int32_t width_px, int32_t height_px);

   /// \brief configures distance-based audio attenuation values.
   /// \param far_distance distance at which far volume is used.
   /// \param far_volume volume at \p far_distance.
   /// \param near_distance distance at which near volume is used.
   /// \param near_volume volume at \p near_distance.
   void addAudioRange(float far_distance, float far_volume, float near_distance, float near_volume);

   /// \brief registers a sample path for this node audio set.
   /// \param sample sample identifier or path.
   void addSample(const std::string& sample);

   /// \brief plays a named sample with an explicit volume multiplier.
   /// \param sample sample identifier or path.
   /// \param volume linear volume multiplier.
   void playSample(const std::string& sample, float volume);

   /// \brief checks overlap between a rectangle and the player bounds.
   /// \param x rectangle left in pixels.
   /// \param y rectangle top in pixels.
   /// \param width rectangle width in pixels.
   /// \param height rectangle height in pixels.
   /// \return true when the rectangle intersects the player.
   bool intersectsPlayer(float x, float y, float width, float height);

   /// \brief checks whether the player is currently marked dead.
   /// \return true when player death state is active.
   bool checkPlayerDead() const;

   /// \brief checks occupancy grid collision along a line segment.
   /// \param x0 start x in physics-grid coordinates.
   /// \param y0 start y in physics-grid coordinates.
   /// \param x1 end x in physics-grid coordinates.
   /// \param y1 end y in physics-grid coordinates.
   /// \return true when the line does not hit blocked cells.
   bool isPhysicsPathClear(int32_t x0, int32_t y0, int32_t x1, int32_t y1) const;

   /// \brief returns current world gravity value.
   /// \return y gravity from the level box2d world.
   float getWorldGravity() const;

   /// \brief creates and stores a weapon from script parameters.
   /// \param weapon_type weapon enum value.
   /// \param fire_interval cooldown in milliseconds between shots.
   /// \param damage_value damage applied by each projectile.
   /// \param gravity_scale gravity multiplier for spawned projectiles.
   /// \param radius projectile radius when using circular projectiles.
   /// \param polygon_points projectile polygon points when using polygon projectiles.
   void addWeaponFromScript(
      WeaponType weapon_type,
      int fire_interval,
      int damage_value,
      float gravity_scale,
      float radius,
      const std::vector<b2Vec2>& polygon_points
   );

   /// \brief sets projectile texture for one weapon slot.
   /// \param weapon_index weapon slot index.
   /// \param path texture path.
   /// \param rect rectangle in pixels.
   void setProjectileTexture(uint32_t weapon_index, const std::string& path, const sf::Rect<int32_t>& rect);

   /// \brief configures projectile animation sheet data for one weapon slot.
   /// \param weapon_index weapon slot index.
   /// \param path sprite sheet path.
   /// \param frame_width frame width in pixels.
   /// \param frame_height frame height in pixels.
   /// \param frame_origin_x local frame origin x in pixels.
   /// \param frame_origin_y local frame origin y in pixels.
   /// \param time_per_frame_s frame duration in seconds.
   /// \param frame_count number of animation frames.
   /// \param frames_per_row number of frames per sheet row.
   /// \param start_frame first frame index.
   void setProjectileAnimation(
      uint32_t weapon_index,
      const std::string& path,
      uint32_t frame_width,
      uint32_t frame_height,
      float frame_origin_x,
      float frame_origin_y,
      float time_per_frame_s,
      uint32_t frame_count,
      uint32_t frames_per_row,
      uint32_t start_frame
   );

   /// \brief starts a one-shot timer that calls lua timeout callback.
   /// \param delay delay in milliseconds.
   /// \param timer_id script-defined timer identifier.
   void startTimer(int32_t delay, int32_t timer_id);

   /// \brief registers projectile-hit animation data for one weapon slot.
   /// \param weapon_index weapon slot index.
   /// \param path hit animation sprite sheet path.
   /// \param frame_width frame width in pixels.
   /// \param frame_height frame height in pixels.
   /// \param time_per_frame_s frame duration in seconds.
   /// \param frame_count number of animation frames.
   /// \param frames_per_row number of frames per sheet row.
   /// \param start_frame first frame index.
   void registerHitAnimation(
      uint32_t weapon_index,
      const std::string& path,
      uint32_t frame_width,
      uint32_t frame_height,
      float time_per_frame_s,
      uint32_t frame_count,
      uint32_t frames_per_row,
      uint32_t start_frame
   );

   /// \brief registers audio samples for projectile-hit animation playback.
   /// \param path hit animation key used for lookup.
   /// \param samples list of sample path and volume pairs.
   void registerHitSamples(const std::string& path, const std::vector<std::pair<std::string, float>>& samples);

   /// \brief plays detonation visuals requested from lua callback parameters.
   /// \param x center x in pixels.
   /// \param y center y in pixels.
   /// \param rings optional ring setup provided by script.
   void playDetonationAnimationFromScript(float x, float y, const std::vector<DetonationAnimation::DetonationRing>& rings);

   /// \brief marks this node dead and destroys its physics body.
   void die();

   /// \brief loads collision shapes from an auxiliary TMX file.
   /// \param tmxFile path to the TMX file.
   void loadShapesFromTmx(const std::string& tmxFile);

   /// \brief loads hitbox definitions from an auxiliary TMX file.
   /// \param tmxFile path to the TMX file.
   void loadHitboxesFromTmx(const std::string& tmxFile);

   /// \brief returns timestamp of the latest hit event.
   /// \return high-resolution timestamp, or std::nullopt when never hit.
   const std::optional<HighResTimePoint> getHitTime() const;

   /// \brief returns damage value from the most recent hit callback.
   /// \return last damage amount received from player attacks.
   int32_t getDamageFromPlayer() const;

   // all functions that 'speak' directly to the lua scripts
   /// \brief calls the script hit callback with incoming damage amount.
   /// \param damage damage amount.
   void luaHit(int32_t damage);

   /// \brief calls the script initialize callback.
   void luaInitialize();

   /// \brief calls the script movedTo callback with current node position.
   void luaMovedTo();

   /// \brief calls the script setStartPosition callback.
   void luaSetStartPosition();

   /// \brief calls the script playerMovedTo callback with player position.
   void luaPlayerMovedTo();

   /// \brief calls the script retrieveProperties callback.
   void luaRetrieveProperties();

   /// \brief pushes a vector path as a lua number table.
   /// \param vec path points in pixels.
   void luaSendPath(const std::vector<sf::Vector2f>& vec);

   /// \brief calls setPath callback with this node patrol path.
   void luaSendPatrolPath();

   /// \brief calls timeout callback after a timer has fired.
   /// \param timerId script-defined timer identifier.
   void luaTimeout(int32_t timerId);

   /// \brief calls update callback with frame delta in seconds.
   /// \param dt elapsed frame time.
   void luaUpdate(const sf::Time& dt);

   /// \brief calls writeProperty callback with one key-value pair.
   /// \param key property key.
   /// \param value property value.
   void luaWriteProperty(const std::string& key, const std::string& value);

   /// \brief calls collisionWithPlayer callback when fixtures collide with player.
   void luaCollisionWithPlayer();

   /// \brief calls smashed callback and marks node as smashed.
   void luaSmashed();

   // property accessors
   /// \brief applies property-dependent native side effects after lua updates.
   void synchronizeProperties();

   /// \brief reads a boolean property from the script property map.
   /// \param key property name.
   /// \param default_value value returned when property is missing.
   /// \return property value or \p default_value.
   bool getPropertyBool(const std::string& key, bool default_value = false);

   /// \brief reads a floating-point property from the script property map.
   /// \param key property name.
   /// \param default_value value returned when property is missing.
   /// \return property value or \p default_value.
   double getPropertyDouble(const std::string& key, double default_value = 0.0);

   /// \brief reads an integer property from the script property map.
   /// \param key property name.
   /// \param default_value value returned when property is missing.
   /// \return property value or \p default_value.
   int64_t getPropertyInt64(const std::string& key, int64_t default_value = 0);

   // box2d related
   /// \brief creates fixtures and collision metadata from current properties and shapes.
   void setupBody();

   /// \brief closes lua state and frees owned native resources.
   void stopScript();

   // members
   int32_t _keys_pressed{0};
   std::string _script_name;
   std::string _name;
   lua_State* _lua_state{nullptr};
   EnemyDescription _enemy_description;
   bool _visible{true};

   // visualization
   sf::Vector2f _start_position_px;
   std::shared_ptr<sf::Texture> _texture;
   std::vector<std::unique_ptr<sf::Sprite>> _sprites;
   std::vector<sf::Vector2f> _sprite_offsets_px;
   sf::Vector2f _position_px;
   std::vector<sf::Vector2f> _movement_path_px;
#ifdef __EMSCRIPTEN__
   std::optional<sf::Shader> _flash_shader;
   std::optional<sf::Shader::UniformLocation> _ul_flash;
#else
   sf::Shader _flash_shader;
#endif
   float _hit_flash{0.0f};
   std::vector<sf::FloatRect> _debug_rects;

   // physics
   b2Body* _body{nullptr};
   b2BodyDef* _body_def{nullptr};
   std::vector<b2Shape*> _shapes_m;
   std::vector<std::shared_ptr<Weapon>> _weapons;

   // damage
   std::vector<Hitbox> _hitboxes;
   std::optional<sf::FloatRect> _bounding_box;
   std::optional<HighResTimePoint> _hit_time;
   int32_t _damage_from_player{0};
   bool _dead{false};
   bool _smashed{false};

   using LuaVariant = std::variant<std::string, int64_t, double, bool>;
   std::unordered_map<std::string, LuaVariant> _properties;
};
