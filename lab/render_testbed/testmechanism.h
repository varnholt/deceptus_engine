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
         sf::Vector2f pos_from_angle_and_distance_px;
         pos_from_angle_and_distance_px.x = std::cos(full_angle_sf.asRadians()) * _distance_factor;
         pos_from_angle_and_distance_px.y = std::sin(full_angle_sf.asRadians()) * _distance_factor;
         _layer->_sprite->setRotation(full_angle_sf);
         _layer->_sprite->setPosition(_pos_px + pos_from_angle_and_distance_px + _offset_px);
      }

      void reset()
      {
         _angle = sf::degrees(0.0f);
         _distance_factor = 1.0f;
         update();
      }

      std::shared_ptr<Layer> _layer;
      sf::Vector2f _pos_px;
      sf::Vector2f _offset_px;
      sf::Angle _angle;
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

   struct EnabledState
   {
      sf::Time _elapsed_time;
   };

   struct ActivatedState
   {
      void resetTime()
      {
         _elapsed_time = sf::seconds(0);
      }
      sf::Time _elapsed_time;
      int32_t _step{0};

      float _speed{0.0f};
      float _acceleration{0.001f};
      float _friction{0.999f};

      sf::Angle _angle_start{};
      sf::Angle _angle_target{};
      bool _has_target_angle = false;
   };

   State _state{State::Disabled};

   std::shared_ptr<sf::Sprite> _socket_sprite;

   sf::RectangleShape _rectangle_;
   sf::CircleShape _origin_shape;


   std::string _filename;
   std::vector<std::shared_ptr<Layer>> _layer_stack;
   std::map<std::string, std::shared_ptr<Layer>> _layers;

   std::array<Side, 4> _pa;

   // std::array<float, 4> _angles;
   float _elapsed{0.0f};
   sf::Vector2f _origin;

   ActivatedState _activated_state;
   EnabledState _enabled_state;
};
