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

private:
   struct Side
   {
      std::shared_ptr<Layer> _layer;
      sf::Vector2f _pos;
      sf::Vector2f _offset;
      sf::Angle _angle;
      sf::Angle _angle_offset;
      float _distance_factor{1.0f};
   };

   enum class State
   {
      Disabled,
      Enabling,
      Enabled,
      Activated
   };

   void load();

   struct EnabledState
   {
      sf::Time _elapsed_time;
      // bool _idle{false};
      // sf::Time _idle_time;
   };

   State _state{State::Enabled};

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

   EnabledState _enabled_state;
};
