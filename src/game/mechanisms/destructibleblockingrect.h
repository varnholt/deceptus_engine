#pragma once

#include "game/level/fixturenode.h"
#include "game/mechanisms/gamemechanism.h"
#include "game/io/gamedeserializedata.h"

#include <SFML/Graphics.hpp>
#include <memory>
#include <optional>
#include <string>

/**
 * DestructibleBlockingRect
 *
 * This mechanism behaves like a solid blocking rectangle that can be damaged
 * by the player's melee attacks.  It blocks the player but never deals
 * damage.  Each hit advances the destruction animation and reduces the hit
 * counter.  When the hit counter reaches zero the mechanism disables its
 * physics body and stops blocking, optionally playing a destroy sound.
 *
 * The following TMX object properties may be used to configure the
 * mechanism:
 *   texture        : string  -- Path relative to the data base path for the
 *                               spritesheet.  Defaults to
 *                               "data/sprites/wooden_planks.png".
 *   frame_width    : int     -- Width in pixels of a single animation frame.
 *                               Defaults to 150.
 *   frame_height   : int     -- Height in pixels of a single animation frame.
 *                               Defaults to 163.
 *   frames         : int     -- Number of frames in the destruction row.
 *                               Defaults to 4.
 *   row            : int     -- Row index of the animation (0-based).  The
 *                               spritesheet is assumed to have at least two
 *                               rows; row 0 might be left-aligned and row 1
 *                               right-aligned.  Defaults to 0.
 *   hits           : int     -- How many hits the object can take before
 *                               collapsing.  Defaults to 4.
 *   hit_sound      : string  -- Sample name to play on each hit (optional).
 *   destroy_sound  : string  -- Sample name to play when destroyed (optional).
 *   z              : int     -- z-index for draw order (optional).
 *
 * The physical blocking area is fixed at 48x96 pixels.  Each frame in the
 * spritesheet may be larger (e.g. 150x163) to allow for overhang; the
 * sprite is drawn anchored to the bottom-left corner of the blocking area.
 */
class DestructibleBlockingRect : public FixtureNode, public GameMechanism
{
public:
    /**
     * Construct the mechanism from the TMX deserialize data.  All
     * configuration is read from the object's properties.
     */
    DestructibleBlockingRect(GameNode* parent, const GameDeserializeData& data);

    /// Return the object name used in the TMX editor.
    [[nodiscard]] std::string_view objectName() const override { return "DestructibleBlockingRect"; }

    /// Draw the sprite to the color and normal render targets.
    void draw(sf::RenderTarget& color, sf::RenderTarget& normal) override;

    /// Update is a no-op as animation frames only advance on hit.
    void update(const sf::Time& dt) override;

    /// Return the current bounding box in pixel coordinates for culling.
    [[nodiscard]] std::optional<sf::FloatRect> getBoundingBoxPx() override;

    /// Collision callbacks for detecting weapon hits on the fixture.
    void beginContact(b2Contact* contact, FixtureNode* other) override;
    void endContact(FixtureNode* other) override;

    /**
     * Callback invoked by the engine or contact handlers when the player
     * hits this mechanism.  Damage and weapon type could be used in
     * future to adjust behaviour; currently a damage of 1 represents a
     * single sword strike.  Marked public so that the engine or
     * higher-level gameplay code can trigger a hit on this mechanism
     * without relying solely on contact detection.
     */
    void onHit(int damage = 1);

private:
    /**
     * Configuration options for this mechanism.  These values are
     * populated from TMX object properties in the constructor and
     * remain constant throughout the lifetime of the mechanism.
     */
    struct Config
    {
        int32_t frame_width  = 150;
        int32_t frame_height = 163;
        int32_t frames       = 4;
        int32_t row          = 0;   ///< 0-based row index
        int32_t max_hits     = 4;
        std::string texture_path {"data/sprites/wooden_planks.png"};
        std::string hit_sound;
        std::string destroy_sound;
        int32_t z_index      = 0;
    };

    /**
     * Dynamic state of the mechanism.  These values change when
     * the mechanism is hit and when it is destroyed.
     */
    struct State
    {
        int32_t hits_left     = 0;
        int32_t current_frame = 0;
        bool dead             = false;
    };

    /// Handle a single hit: decrement hit counter, advance the animation,
    /// and check for destruction.  Called from onHit() and contact logic.
    void hit();

    /// Disable the physics body and flag the mechanism as dead.  Plays
    /// destroy sound if configured and prevents further collisions.
    void destroy();

    // configuration and state
    Config _config;
    State _state;

    // rendering
    std::shared_ptr<sf::Texture> _texture;
    std::shared_ptr<sf::Texture> _normal_map;
    std::unique_ptr<sf::Sprite> _sprite;
    sf::FloatRect _rect_px;

    // physics
    b2Body* _body { nullptr };
    b2PolygonShape _shape;
};