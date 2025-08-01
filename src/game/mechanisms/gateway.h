#pragma once

#include <SFML/Graphics.hpp>
#include <map>

#include "framework/image/layer.h"
#include "game/animation/animation.h"
#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

class Gateway : public GameMechanism, public GameNode
{
public:
   Gateway(GameNode* parent = nullptr);
   virtual ~Gateway();
   std::string_view objectName() const override;
   virtual void draw(sf::RenderTarget& target, sf::RenderTarget& normal);
   virtual void update(const sf::Time& dt);

   void setup(const GameDeserializeData& data);
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   void setTargetId(const std::string& destination_gateway_id);

private:
   enum class State
   {
      Disabled,
      Enabling,
      Enabled
   };

   struct Side
   {
      void update();
      void reset();

      std::shared_ptr<Layer> _layer;
      sf::Vector2f _pos_px;
      sf::Vector2f _offset_px;
      sf::Angle _angle{_base_angle};
      sf::Angle _angle_offset;
      float _distance_factor{1.0f};
   };

   struct Eye
   {
      enum class IrisState
      {
         Asleep,
         Awake,
         Idle
      };

      Eye(const sf::Vector2f& center);

      void draw(sf::RenderTarget& target);
      void update(const sf::Time& dt, State state);

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

   struct PortalState
   {
      sf::Time _elapsed_time;
      void resetTime();
   };

   struct EnabledState : PortalState
   {
      float _frequency{1.0f};
      float _amplitude{2.8f};
      float _offset{1.0f};
      float _irregularity{3.0f};
      float _distances_when_activated{0.0f};
   };

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

   void setSidesVisible(std::array<Side, 4>& sides, bool visible);

   bool checkPlayerAtGateway() const;

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
   void loadNoiseTexture(const std::string& filename);
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
