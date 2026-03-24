#pragma once

#include "framework/math/pathinterpolation.h"
#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

#include "box2d/box2d.h"
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <filesystem>

struct TmxLayer;
struct TmxObject;
struct TmxTileSet;

/// \brief implements a kinematic moving platform that follows a polyline path.
class MovingPlatform : public GameMechanism, public GameNode
{
public:
   /// \brief creates a moving platform mechanism.
   /// \param parent parent node in the scene graph.
   MovingPlatform(GameNode* parent);

   /// \brief returns the mechanism registry name.
   /// \return string view containing `MovingPlatform`.
   std::string_view objectName() const override;

   /// \brief initializes path interpolation, sprites, and the kinematic box2d body from TMX data.
   /// \param data deserialize context containing TMX object and world.
   void setup(const GameDeserializeData& data);

   // static void deserialize(const std::shared_ptr<TmxObject>& tmx_object);
   // static std::vector<std::shared_ptr<GameMechanism>> merge(GameNode* parent, const GameDeserializeData& data);
   // static void link(const std::vector<std::shared_ptr<GameMechanism>>& platforms, const GameDeserializeData& data);

   /// \brief draws platform sprites to color and optional normal render targets.
   /// \param color color render target.
   /// \param normal normal render target.
   void draw(sf::RenderTarget& color, sf::RenderTarget& normal) override;

   /// \brief updates path movement, enable-ramp lag, player coupling, and wheel animations.
   /// \param dt elapsed frame time.
   void update(const sf::Time& dt) override;

   /// \brief returns the precomputed area covered by this platform path.
   /// \return platform bounds in pixels.
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   /// \brief creates the kinematic platform body and collision fixture.
   /// \param world shared box2d world.
   void setupBody(const std::shared_ptr<b2World>& world);

   /// \brief appends one tile sprite to the platform sprite list.
   /// \param sprite sprite to append.
   void addSprite(const sf::Sprite& sprite);

   /// \brief returns the underlying platform box2d body.
   /// \return non-owning pointer to the kinematic body.
   b2Body* getBody();

   /// \brief toggles platform movement and updates lever lag transition state.
   /// \param enabled true to accelerate towards active motion.
   void setEnabled(bool enabled) override;

   /// \brief returns the polyline path points in pixel space.
   /// \return path points in pixel coordinates.
   const std::vector<sf::Vector2f>& getPixelPath() const;

   /// \brief returns frame-to-frame x displacement based on body position history.
   /// \return current x delta since previous update.
   float getDx() const;

private:
   /// \brief initializes the body transform from stored tile coordinates.
   void setupTransform();

   /// \brief smooths movement startup and shutdown after lever state changes.
   /// \param dt elapsed frame time.
   void updateLeverLag(const sf::Time& dt);

   std::shared_ptr<sf::Texture> _texture_map;
   std::shared_ptr<sf::Texture> _normal_map;

   std::vector<sf::Sprite> _sprites;
   int32_t _animated_tile_index_0 = 0;
   int32_t _animated_tile_index_1 = 0;
   float _animation_elapsed = 0.0f;
   b2Body* _body = nullptr;
   sf::Vector2i _tile_positions;
   int32_t _platform_width_tl = 0;
   float _lever_lag = 0.0f;
   bool _initialized = false;
   PathInterpolation<b2Vec2> _interpolation;
   b2Vec2 _velocity{};
   std::vector<sf::Vector2f> _pixel_path;
   sf::FloatRect _rect;
   sf::Vector2f _pos;
   sf::Vector2f _pos_prev;
};
