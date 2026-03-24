#pragma once

// Bubble cube
//
// - You can stay on top of it as long as you want.
// - Once you jump off/ stop colliding with it, it vanishes "pop animation"
// - Cube reappears after n seconds.
// - Basically is a 1-jump-platform before vanishes.

class GameNode;
struct TmxObject;

#include "game/io/gamedeserializedata.h"
#include "game/level/fixturenode.h"
#include "game/mechanisms/gamemechanism.h"

#include "box2d/box2d.h"

#include <filesystem>

/// \brief moving bubble platform that pops after specific player interactions, then respawns.
class BubbleCube : public FixtureNode, public GameMechanism
{
public:
   /// \brief creates the bubble cube body, prismatic joint, sprite, and behavior settings.
   /// \param parent parent node in the scene graph.
   /// \param data deserialize context with tmx properties and world access.
   BubbleCube(GameNode* parent, const GameDeserializeData& data);
   /// \brief returns the mechanism type identifier.
   /// \return non-owning string view with value "BubbleCube".
   std::string_view objectName() const override;

   /// \brief draws the current bubble animation frame.
   /// \param target render target.
   /// \param normal normal render target.
   void draw(sf::RenderTarget& target, sf::RenderTarget& normal) override;
   /// \brief updates motion, contact logic, popping conditions, and respawn state.
   /// \param dt elapsed frame time.
   void update(const sf::Time& dt) override;
   /// \brief returns the original platform bounds in pixel coordinates.
   /// \return rectangle for culling and gameplay checks.
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   /// \brief handles begin-contact callbacks.
   /// \param contact box2d contact object.
   /// \param other other fixture node involved in the contact.
   void beginContact(b2Contact* contact, FixtureNode* other);
   /// \brief handles end-contact callbacks.
   /// \param other other fixture node that ended contact.
   void endContact(FixtureNode* other);

private:
   /// \brief synchronizes sprite and translated bounds with the box2d body position.
   void updatePosition();
   /// \brief handles timed respawn and fade-in after the bubble has popped.
   void updateRespawnCondition();
   /// \brief evaluates whether current flags require popping the platform.
   void updatePoppedCondition();
   /// \brief checks whether the bubble is pressed into surrounding bodies and should pop.
   void updatePopOnCollisionCondition();
   /// \brief tracks standing time and flags a pop when max contact duration is exceeded.
   /// \param dt elapsed frame time.
   void updateMaxDurationCondition(const sf::Time& dt);
   /// \brief updates whether the player's foot sensor overlaps the bubble top area.
   void updateFootSensorContact();
   /// \brief detects jump-off events from this bubble on the jump start frame.
   void updateJumpedOffPlatformCondition();
   /// \brief updates prismatic motor speed for bobbing, sinking, and retracting behavior.
   /// \param dt elapsed frame time.
   void updateMotorSpeed(const sf::Time& dt);

   /// \brief pops the bubble, disables its body, and resets transient flags.
   void pop();

   int32_t _instance_id = 0;
   float _x_px = 0.0f;
   float _y_px = 0.0f;
   sf::FloatRect _original_rect_px;
   sf::FloatRect _translated_rect_px;
   sf::FloatRect _foot_collision_rect_px;
   sf::FloatRect _jump_off_collision_rect_px;
   float _elapsed_s = 0.0f;
   float _pop_elapsed_s = 0.0f;
   sf::Time _pop_time;
   sf::Time _respawn_time;
   float _alpha = 1.0f;
   bool _popped = false;
   bool _exceeded_max_contact_duration = false;
   bool _collided_with_surrounding_areas = false;
   bool _jumped_off_this_platform = false;
   std::optional<size_t> _colliding_body_count;

   bool _lost_foot_contact = false;
   bool _foot_sensor_rect_intersects = false;
   bool _foot_sensor_rect_intersects_previous = false;

   // settings
   float _pop_time_respawn_s = 3.0f;
   float _animation_offset_s = 0.0f;
   float _mapped_value_normalized = 0.0f;
   float _move_down_velocity = 0.5f;
   float _move_up_velocity = -0.3f;
   float _contact_duration_s = 0.0f;
   std::optional<float> _maximum_contact_duration_s;

   // sf
   std::shared_ptr<sf::Texture> _texture;
   std::unique_ptr<sf::Sprite> _sprite;

   // b2d
   b2Body* _body = nullptr;
   b2Fixture* _fixture = nullptr;
   b2Vec2 _position_m;
   b2PolygonShape _shape;

   // spring based approach
   b2BodyDef _anchor_def;
   b2Body* _anchor_body{nullptr};
   b2EdgeShape _anchor_a_shape;
   b2PrismaticJoint* _joint{nullptr};
   float _motor_time_s{0.0f};
   float _motor_speed{0.0f};
   /// \brief advances the animation phase used by the regular floating animation.
   void updateSpriteIndex();
};
