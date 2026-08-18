#ifndef PLAYERHARPOON_H
#define PLAYERHARPOON_H

#include <SFML/Graphics.hpp>

#include "box2d/box2d.h"
#include "game/player/playercontrols.h"
#include "game/player/playerropehold.h"

#include <memory>
#include <optional>
#include <vector>

/// \brief harpoon that is shot into level geometry and carries the player on a simulated rope.
class PlayerHarpoon
{
public:
   /// \brief per-frame harpoon input gathered from player state and controls.
   struct HarpoonInput
   {
      std::shared_ptr<b2World> _world;
      std::shared_ptr<PlayerControls> _controls;
      b2Body* _player_body{nullptr};
      bool _points_to_left{false};
      bool _harpoon_button_pressed{false};
      bool _jump_button_pressed{false};
      bool _move_left_pressed{false};
      bool _move_right_pressed{false};
      bool _in_water{false};
      bool _on_ground{false};
      bool _dead{false};
      bool _analogue_aim{false};  //!< true while a controller is the input device, so the aim is set directly

      //!< the rope already carries the player, so the harpoon must not carry him as well
      bool _carried_elsewhere{false};
   };

   /// \brief advances the harpoon state machine, shoots or releases the rope and applies swing control.
   /// \param dt elapsed frame time.
   /// \param input harpoon command, environment state and player body.
   void update(const sf::Time& dt, const HarpoonInput& input);

   /// \brief draws the rope and the hook head.
   /// \param color color render target.
   /// \param normal normal-map render target.
   /// \param states render states to apply (used in WASM to carry the level view).
   void draw(sf::RenderTarget& color, sf::RenderTarget& normal, const sf::RenderStates& states = {});

   /// \brief destroys all rope bodies and joints and returns to the idle state.
   void reset();

   /// \brief reports whether the rope currently carries the player.
   /// \return true while the player hangs on the rope.
   bool isAttached() const;

   /// \brief reports whether the player still keeps the momentum gained from a released swing.
   /// \return true while the post-release grace period is running.
   bool isReleaseGraceActive() const;

private:
   enum class State
   {
      Idle,     //!< no harpoon in the level
      Flying,   //!< hook is travelling towards its target
      Attached  //!< rope is built and carries the player
   };

   /// \brief holds the aim while the fire button is down and shoots once it is released.
   /// \param dt elapsed frame time.
   /// \param input current harpoon input.
   void updateAiming(const sf::Time& dt, const HarpoonInput& input);

   /// \brief takes up and down away from the rest of the game while the harpoon owns them.
   /// \param input current harpoon input.
   /// \note while aiming, up and down sweep the aim angle, while attached they reel the rope. crouching,
   ///       dropping through platforms and opening dialogues must not happen from the same key then, and
   ///       none of those need to know that the harpoon exists.
   void updateVerticalKeyClaim(const HarpoonInput& input);

   /// \brief draws the aim angle indicator around the player.
   /// \param color color render target.
   /// \param states render states to apply.
   void drawAimIndicator(sf::RenderTarget& color, const sf::RenderStates& states);

   /// \brief casts a ray into the shoot direction and starts the hook flight.
   /// \param input current harpoon input.
   void shoot(const HarpoonInput& input);

   /// \brief builds the rope chain between the anchor and the player body.
   /// \param input current harpoon input.
   void createRope(const HarpoonInput& input);

   /// \brief loads the rope and hook textures on first use.
   void loadTextures();

   /// \brief builds a textured quad rotated into a direction, used for the hook.
   /// \param texture sprite texture, drawn pointing along +x.
   /// \param center_px sprite center in pixels.
   /// \param direction normalized direction the sprite points to.
   /// \param mirrored true to mirror the sprite instead of turning it around.
   /// \return four vertices forming a triangle strip.
   static std::vector<sf::Vertex>
   buildSpriteStrip(const sf::Texture& texture, const sf::Vector2f& center_px, const b2Vec2& direction, bool mirrored);

   /// \brief converts a box2d vector into an sfml vector without changing its scale.
   /// \param vector box2d vector.
   /// \return the same components as an sfml vector.
   static sf::Vector2f toVector2f(const b2Vec2& vector);

   /// \brief creates one rope segment body.
   /// \param center_m segment center in box2d world units.
   /// \param angle segment angle in radians.
   /// \param colliding false for the segment stuck inside the hooked surface.
   /// \return the created segment body.
   b2Body* createSegment(const b2Vec2& center_m, float angle, bool colliding);

   /// \brief hangs the player off the current rope end.
   /// \param player_body player body the rope end is jointed to.
   /// \note the anchor sits at the far end of the last segment, which is where the rope actually ends.
   void attachPlayer(b2Body* player_body);

   /// \brief shortens or lengthens the rope while up or down is held.
   /// \param dt elapsed frame time.
   /// \param input current harpoon input.
   void updateRopeLength(const sf::Time& dt, const HarpoonInput& input);

   /// \brief removes the rope segment closest to the player and re-attaches the player.
   /// \param input current harpoon input.
   void removeLastSegment(const HarpoonInput& input);

   /// \brief appends a rope segment at the player end and re-attaches the player.
   /// \param input current harpoon input.
   void appendSegment(const HarpoonInput& input);

   /// \brief returns the current overall rope length.
   /// \return length of all segments plus the adjustable player link in box2d units.
   float readRopeLength() const;

   /// \brief destroys the rope chain, its joints and the anchor body.
   void destroyRope();

   /// \brief releases the rope and starts the momentum grace period.
   void release();

   /// \brief turns the left and right input into the swing control the shared hold applies.
   /// \param input current harpoon input.
   void applySwingControl(const HarpoonInput& input);

   State _state{State::Idle};
   bool _aiming{false};
   bool _fire_locked_until_released{false};  //!< keeps releasing the rope from starting a new aim right away
   float _aim_angle_deg{0.0f};               //!< relative to the facing direction, positive points up
   b2Vec2 _aim_direction{1.0f, 0.0f};        //!< the aim angle resolved against the facing direction
   sf::Vector2f _aim_origin_px;
   bool _harpoon_button_was_pressed{false};
   bool _jump_button_was_pressed{false};
   PlayerRopeHold _hold;  //!< carries the player, swings him and owns up and down while the rope is his

   b2Vec2 _shoot_position_m{0.0f, 0.0f};
   b2Vec2 _shoot_direction_m{0.0f, 0.0f};
   b2Vec2 _anchor_position_m{0.0f, 0.0f};
   bool _target_found{false};
   float _flight_length_m{0.0f};
   float _flight_travelled_m{0.0f};
   float _release_grace_remaining_s{0.0f};

   float _segment_length_m{0.0f};      //!< kept so segments added while reeling out match the existing ones
   float _segment_density{0.0f};       //!< derived from the player mass when the rope is created
   float _player_link_length_m{0.0f};  //!< length of the adjustable last link, always within one segment length

   std::shared_ptr<sf::Texture> _rope_texture;
   std::shared_ptr<sf::Texture> _hook_texture;

   std::shared_ptr<b2World> _world;
   b2Body* _anchor_body{nullptr};
   std::vector<b2Body*> _rope_bodies;
   std::vector<b2Joint*> _rope_joints;
};

#endif  // PLAYERHARPOON_H
