#pragma once

#include <SFML/Graphics.hpp>
#include <map>

#include "framework/image/layer.h"
#include "game/animation/animation.h"
#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

/// \brief controls a gateway that activates through staged animation and teleports the player.
class Gateway : public GameMechanism, public GameNode
{
public:
   /// \brief creates a gateway mechanism and preloads gateway sound samples.
   /// \param parent parent node in the scene graph.
   Gateway(GameNode* parent = nullptr);

   /// \brief unregisters this gateway from the global gateway registry.
   virtual ~Gateway();

   /// \brief returns the mechanism registry name.
   /// \return string view containing `Gateway`.
   std::string_view objectName() const override;

   /// \brief draws gateway layers, rotating side parts, void shader effect, and eye animation.
   /// \param target render target.
   /// \param normal normal-map render target (unused).
   virtual void draw(sf::RenderTarget& target, sf::RenderTarget& normal);

   /// \brief updates activation state, side animations, eye tracking, and optional teleport use.
   /// \param dt elapsed frame time.
   virtual void update(const sf::Time& dt);

   /// \brief initializes gateway geometry, visuals, shader resources, and object properties.
   /// \param data deserialize context with TMX object data and resource paths.
   void setup(const GameDeserializeData& data);

   /// \brief returns bounds for mechanism queries.
   /// \return `std::nullopt` because gateway interaction uses internal checks.
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   /// \brief sets the destination gateway object id used by teleport.
   /// \param destination_gateway_id object id of the destination gateway.
   void setTargetId(const std::string& destination_gateway_id);

private:
   enum class State
   {
      Disabled,
      Enabling,
      Enabled
   };

   /// \brief stores one rotating side element and its transform state.
   struct Side
   {
      /// \brief updates the side sprite transform from angle, distance, and offset.
      void update();

      /// \brief resets side transform values to the default idle configuration.
      void reset();

      std::shared_ptr<Layer> _layer;
      sf::Vector2f _pos_px;
      sf::Vector2f _offset_px;
      sf::Angle _angle{_base_angle};
      sf::Angle _angle_offset;
      float _distance_factor{1.0f};
   };

   /// \brief handles gateway eye animations and iris state transitions.
   struct Eye
   {
      enum class IrisState
      {
         Asleep,
         Awake,
         Idle
      };

      /// \brief creates eye visuals centered on the given gateway position.
      /// \param center center position of the gateway in pixels.
      Eye(const sf::Vector2f& center);

      /// \brief draws the currently active eye animation.
      /// \param target render target.
      void draw(sf::RenderTarget& target);

      /// \brief updates iris state, animation playback, and gaze tracking towards the player.
      /// \param dt elapsed frame time.
      /// \param state current gateway activation state.
      void update(const sf::Time& dt, State state);

      /// \brief starts the eye wake-up sequence.
      void wakeUp();

      sf::Vector2f _eye_pos_px;
      sf::Vector2f _center_pos_px;

      std::shared_ptr<sf::Texture> _texture;
      std::unique_ptr<sf::Sprite> _sprite;

      std::shared_ptr<Animation> _eye_spawn;
      std::shared_ptr<Animation> _eye_iris_spawn;
      std::shared_ptr<Animation> _eye_iris_idle;
      std::shared_ptr<Animation> _eye_iris_idle_blink;
      std::shared_ptr<Animation> _eye_iris_idle_ref;

      std::array<std::shared_ptr<Animation>, 4> _animations;

      IrisState _iris_state{IrisState::Asleep};
      sf::Time _wake_time;
      static constexpr auto _wake_duration_s{2.0f};

      State _state = State::Disabled;
   };

   /// \brief base state container with shared elapsed-time handling.
   struct PortalState
   {
      sf::Time _elapsed_time;

      /// \brief resets elapsed time for the current step.
      void resetTime();
   };

   /// \brief stores oscillation parameters used while the gateway is fully enabled.
   struct EnabledState : PortalState
   {
      float _frequency{1.0f};
      float _amplitude{2.8f};
      float _offset{1.0f};
      float _irregularity{3.0f};
      float _distances_when_activated{0.0f};
   };

   /// \brief stores parameters and intermediate values for the enabling animation sequence.
   struct ActivatedState : PortalState
   {
      int32_t _step{0};
      sf::Angle _angle_start{};
      sf::Angle _angle_target{};
      bool _has_target_angle = false;
      float _speed{0.0f};

      // settings
      float _acceleration{0.02f};
      float _friction{0.9f};
      int32_t _rise_height_px{60};
      int32_t _extend_distance_px{50};
      float _spinback_duration_s{1.0f};
      float _retract_duration_s{1.0};
      float _rotate_right_duration_s{2.0f};
      float _rotate_left_duration_s{3.0f};
      float _rotate_speed_max{1.0f};
      float _fade_duration_s{2.0f};
   };

   /// \brief toggles visibility for all side layers in an array.
   /// \param sides side array to update.
   /// \param visible true to mark layers as visible.
   void setSidesVisible(std::array<Side, 4>& sides, bool visible);

   /// \brief checks whether the player's pixel position is inside the gateway rectangle.
   /// \return true when the player is inside the gateway area.
   bool checkPlayerAtGateway() const;

   /// \brief starts teleport usage with a fade transition when a valid target gateway exists.
   void use();

   State _state{State::Disabled};

   std::shared_ptr<sf::Sprite> _sprite_socket;
   std::shared_ptr<Layer> _layer_background_inactive;
   std::shared_ptr<Layer> _layer_background_active;

   sf::RectangleShape _rect_shape;
   sf::CircleShape _origin_shape;

   std::string _filename;
   std::vector<std::shared_ptr<Layer>> _layer_stack;
   std::map<std::string, std::shared_ptr<Layer>> _layers;

   std::array<Side, 4> _pa;
   std::array<Side, 4> _pi;
   static constexpr sf::Angle _base_angle{sf::degrees(45.0f)};

   float _elapsed{0.0f};
   sf::Vector2f _origin;
   sf::FloatRect _rect;

   ActivatedState _activated_state;
   EnabledState _enabled_state;
   bool _player_intersects{false};
   bool _in_use{false};
   std::string _target_id;

   // shader
   /// \brief loads and configures the repeating noise texture used by the void shader.
   /// \param filename texture filename to load.
   void loadNoiseTexture(const std::string& filename);

   /// \brief renders the animated shader-based void effect at the gateway center.
   /// \param target render target.
   void drawVoid(sf::RenderTarget& target);

   sf::Shader _shader;
   std::unique_ptr<sf::RenderTexture> _shader_texture;
   std::unique_ptr<sf::Sprite> _shader_sprite;
   float _radius_factor = 1.0f;
   float _shader_alpha = 0.7f;
   float _void_alpha = 0.0f;
   float _time_factor = 4.0f;
   float _noise_scale = 10.0;
   sf::Vector3f _swirl_color{0.0f, 0.5f, 0.8f};
   sf::Texture _noise_texture;
   std::string _default_texture_path{"data/effects/gabor_6.png"};

   // flowfield
   std::optional<std::string> _flowfield_reference_id;
   std::optional<std::string> _flowfield_texture;

   // eye
   std::unique_ptr<Eye> _eye;
};
