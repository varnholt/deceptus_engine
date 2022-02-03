#pragma once

#include <SFML/Graphics.hpp>
#include <cstdint>

#include "constants.h"


class CameraPanorama
{
public:

   static CameraPanorama& getInstance();

   void update();

   void processKeyPressedEvents(const sf::Event& event);
   void processKeyReleasedEvents(const sf::Event& event);

   void updateLookState(Look look, bool enable);
   void updateLookVector(const sf::Vector2f& desired);

   bool isLookActive() const;
   const sf::Vector2f& getLookVector() const;


private:

   CameraPanorama() = default;

   int32_t _look_state = static_cast<int32_t>(Look::Inactive);
   sf::Vector2f _look_vector;

   static CameraPanorama __instance;
};

