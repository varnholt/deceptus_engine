#pragma once

#include "framework/joystick/gamecontrollerinfo.h"
#include <functional>
#include <SFML/Graphics.hpp>

class PlayerControls
{

   public:
   PlayerControls() = default;

   void update(const sf::Time& dt);

   using KeypressedCallback = std::function<void(sf::Keyboard::Key)>;
   void addKeypressedCallback(const KeypressedCallback& callback);

   bool hasFlag(int32_t flag) const;

   void keyboardKeyPressed(sf::Keyboard::Key key);
   void keyboardKeyReleased(sf::Keyboard::Key key);
   void forceSync();

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

   bool wasMoving() const;
   void setWasMoving(bool was_moving);

   bool wasMovingLeft() const;
   void setWasMovingLeft(bool was_moving_left);

   bool wasMovingRight() const;
   void setWasMovingRight(bool was_moving_right);

   bool changedToIdle() const;
   bool changedToMoving() const;


private:

   GameControllerInfo _joystick_info;

   int32_t _keys_pressed = 0;

   bool _was_moving = false;
   bool _was_moving_left = false;
   bool _was_moving_right = false;

   std::vector<KeypressedCallback> _keypressed_callbacks;
};


