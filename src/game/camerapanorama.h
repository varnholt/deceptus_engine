#pragma once

#include <SFML/Graphics.hpp>
#include <cstdint>

#include "constants.h"


class CameraPanorama
{
public:

   static CameraPanorama& getInstance();

   void update();
   void updateLookState(Look look, bool enable);
   void updateLookVector(const sf::Vector2f& desired);

   void processKeyPressedEvents(const sf::Event& event);
   void processKeyReleasedEvents(const sf::Event& event);

   bool isLookActive() const;
   const sf::Vector2f& getLookVector() const;


private:
   CameraPanorama() = default;

   // clang-format off
   sf::Vector2f computeDesiredLookVector(
      bool looking_up,
      bool looking_down,
      bool looking_left,
      bool looking_right,
      bool can_look_up,
      bool can_look_down,
      bool can_look_left,
      bool can_look_right,
      float x_relative,
      float y_relative
   ) const;
   // clang-format on

   int32_t _look_state = static_cast<int32_t>(Look::Inactive);
   sf::Vector2f _look_vector;
};

