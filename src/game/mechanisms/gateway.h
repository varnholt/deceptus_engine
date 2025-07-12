#pragma once

#include <SFML/Graphics.hpp>
#include <map>

#include "framework/image/layer.h"
#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

class Gateway : public GameMechanism, public GameNode, public std::enable_shared_from_this<Gateway>
{
public:
   Gateway(GameNode* parent = nullptr);
   virtual ~Gateway();
   virtual void draw(sf::RenderTarget& target, sf::RenderTarget& normal);
   virtual void update(const sf::Time& dt);

   void setup(const GameDeserializeData& data);
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   void setTargetId(const std::string& destination_gateway_id);

private:
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

   enum class State
   {
      Disabled,
      Enabling,
      Enabled
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

   std::shared_ptr<Gateway> findOtherInstance(const std::string& id) const;

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
};
