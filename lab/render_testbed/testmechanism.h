#pragma once

#include <SFML/Graphics.hpp>
#include <map>

#include "../../src/framework/image/layer.h"

class TestMechanism
{
public:
   TestMechanism();
   virtual ~TestMechanism() = default;
   virtual void draw(sf::RenderTarget& target, sf::RenderTarget& normal);
   virtual void update(const sf::Time& dt);

   void chooseNextState();

private:
   struct Side
   {
      void update()
      {
         const auto full_angle_sf = _angle + _angle_offset;
         sf::Vector2f pos_from_angle_and_distance_px{
            std::cos(full_angle_sf.asRadians()) * _distance_factor,  //
            std::sin(full_angle_sf.asRadians()) * _distance_factor
         };
         _layer->_sprite->setRotation(full_angle_sf);
         _layer->_sprite->setPosition(_pos_px + pos_from_angle_and_distance_px + _offset_px - sf::Vector2f{1.0f, 1.0f});
      }

      void reset()
      {
         _angle = _base_angle;
         _distance_factor = 1.0f;
         _offset_px = {};
         update();
      }

      std::shared_ptr<Layer> _layer;
      sf::Vector2f _pos_px;
      sf::Vector2f _offset_px;
      sf::Angle _angle{_base_angle};
      sf::Angle _angle_offset;
      float _distance_factor{1.0f};
   };

   enum class State
   {
      Disabled,
      Enabling,
      Enabled
   };

   void load();
   void drawEditor();

   struct PortalState
   {
      sf::Time _elapsed_time;
      void resetTime()
      {
         _elapsed_time = sf::seconds(0);
      }
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
      float _acceleration{0.01f};
      float _friction{0.9f};
      int32_t _rise_height_px{60};
      int32_t _extend_distance_px{50};
      float _spinback_duration_s{1.0f};
      float _retract_duration_s{1.0};
      float _rotate_right_duration_s{2.0f};
      float _rotate_left_duration_s{3.0f};
      float _rotate_speed_max{0.2f};
      float _fade_duration_s{2.0f};
   };

   void setSidesVisible(std::array<Side, 4>& sides, bool visible);

   State _state{State::Disabled};

   std::shared_ptr<sf::Sprite> _sprite_socket;
   std::shared_ptr<Layer> _layer_background_inactive;
   std::shared_ptr<Layer> _layer_background_active;

   sf::RectangleShape _rectangle_;
   sf::CircleShape _origin_shape;

   std::string _filename;
   std::vector<std::shared_ptr<Layer>> _layer_stack;
   std::map<std::string, std::shared_ptr<Layer>> _layers;

   std::array<Side, 4> _pa;
   std::array<Side, 4> _pi;
   static constexpr sf::Angle _base_angle{sf::degrees(45.0f)};

   // std::array<float, 4> _angles;
   float _elapsed{0.0f};
   sf::Vector2f _origin;

   ActivatedState _activated_state;
   EnabledState _enabled_state;

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
   std::string _default_texture_path{"data/Gabor 6 - 128x128.png"};
};
