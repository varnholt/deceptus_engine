#pragma once

#include "joystick/gamecontrollerinfo.h"
#include <functional>
#include <SFML/Graphics.hpp>

class PlayerControls
{
public:
   PlayerControls() = default;

   using KeypressedCallback = std::function<void(sf::Keyboard::Key)>;
   void addKeypressedCallback(const KeypressedCallback& callback);

   bool hasFlag(int32_t flag) const;

   void keyboardKeyPressed(sf::Keyboard::Key key);
   void keyboardKeyReleased(sf::Keyboard::Key key);

   int getKeysPressed() const;
   void setKeysPressed(int keys);

   bool isLookingAround() const;
   bool isControllerUsed() const;
   bool isControllerButtonPressed(int buttonEnum) const;
   bool isFireButtonPressed() const;
   bool isJumpButtonPressed() const;

   bool isMovingRight() const;
   bool isMovingLeft() const;
   bool isMoving() const;

   const GameControllerInfo& getJoystickInfo() const;
   void setJoystickInfo(const GameControllerInfo& joystickInfo);

private:
   GameControllerInfo mJoystickInfo;
   int mKeysPressed = 0;

   std::vector<KeypressedCallback> mKeypressedCallbacks;
};


